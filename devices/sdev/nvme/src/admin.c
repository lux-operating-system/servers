/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * nvme: Device driver for NVMe SSDs
 */

/* Implementation of NVMe Admin Commands */

#include <liblux/liblux.h>
#include <liblux/sdev.h>
#include <nvme/nvme.h>
#include <nvme/registers.h>
#include <nvme/nvmcmd.h>
#include <sys/lux/lux.h>
#include <string.h>
#include <stdlib.h>

static const char *controllerType[] = {
    "I/O controller",
    "discovery controller",
    "admin controller"
};

/* nvmeIdentify(): identifies an NVMe device
 * params: drive - pointer to the controller structure
 * returns: zero on success
 */

int nvmeIdentify(NVMEController *drive) {
    // allocate memory for the structures
    drive->idPhys = pcontig(0, sizeof(NVMEIdentification), 0);
    if(!drive->idPhys) return -1;

    drive->id = (NVMEIdentification *) mmio(drive->idPhys, sizeof(NVMEIdentification), MMIO_R | MMIO_W | MMIO_CD | MMIO_ENABLE);
    if(!drive->id) return -1;
    memset(drive->id, 0, sizeof(NVMEIdentification));

    // construct the command structure
    NVMECommonCommand cmd;
    memset(&cmd, 0, sizeof(NVMECommonCommand));

    cmd.dword0 = NVME_ADMIN_IDENTIFY;
    cmd.dword0 |= (0x1234 << 16);
    cmd.dataLow = drive->idPhys;    // direct PRP because we're not crossing a page boundary
    cmd.dword10 = 0x01;             // CNS 0x01 = identify controller

    // submit the command
    nvmeSubmit(drive, 0, &cmd);
    if(!nvmePoll(drive, 0, 0x1234, 20)) {
        luxLogf(KPRINT_LEVEL_WARNING, "- timeout while identifying drive, aborting...\n");
        return -1;
    }

    memcpy(drive->serial, drive->id->serial, 20);
    memcpy(drive->model, drive->id->model, 40);
    memcpy(drive->qualifiedName, drive->id->qualifiedName, 256);
    drive->vendor = drive->id->pciVendor;

    luxLogf(KPRINT_LEVEL_DEBUG, "- model: %s\n", drive->model);

    int ctrlType = drive->id->controllerType-1;
    if(ctrlType >= 0 && ctrlType <= 2) {
        luxLogf(KPRINT_LEVEL_DEBUG, "- controller type %d (%s)\n", drive->id->controllerType, controllerType[ctrlType]);
    } else {
        luxLogf(KPRINT_LEVEL_WARNING, "- controller type %d (unimplemented), aborting...\n", drive->id->controllerType);
    }

    // following the recommended setup procedure of the NVMe base spec, we now
    // need to identify the command sets (CNS 0x1C) if CAP.IOCSS is enabled,
    // and then if that's successful we need to issue the set features command
    // specifying which I/O command set we will be using
    // for the sake of this driver, we will only be implementing the NVM I/O
    // command set

    uint64_t cap = nvmeRead64(drive, NVME_CAP);
    if(cap & NVME_CAP_IO_CMDS) {
        memset(&cmd, 0, sizeof(NVMECommonCommand));
        cmd.dword0 = NVME_ADMIN_IDENTIFY;
        cmd.dword0 |= (0xDEAD << 16);
        cmd.dataLow = drive->idPhys;
        cmd.dword10 = 0x1C;         // CNS 0x1C = identify command sets

        nvmeSubmit(drive, 0, &cmd);
        if(!nvmePoll(drive, 0, 0xDEAD, 20)) {
            luxLogf(KPRINT_LEVEL_WARNING, "- timeout while identifying drive, aborting...\n");
            return -1;
        }

        uint64_t *ioID = (uint64_t *) drive->id;
        int cmdProfile = -1;
        for(int i = 0; i < 512; i++) {
            if(ioID[i] & NVME_NVM_COMMAND_SET) {
                cmdProfile = i;
                break;
            }
        }

        if(cmdProfile < 0) {
            luxLogf(KPRINT_LEVEL_WARNING, "- device does not implement NVM command set, aborting...\n");
            return -1;
        }

        luxLogf(KPRINT_LEVEL_DEBUG, "- using I/O command set profile %d: %s%s%s%s\n", cmdProfile,
            ioID[cmdProfile] & NVME_NVM_COMMAND_SET ? "NVM " : "",
            ioID[cmdProfile] & NVME_KV_COMMAND_SET ? "key-value " : "",
            ioID[cmdProfile] & NVME_ZONED_NS_COMMAND_SET ? "zoned " : "",
            ioID[cmdProfile] & NVME_LOCAL_COMMAND_SET ? "local" : "");

        // send an admin set features command to enable the NVM I/O command set
        memset(&cmd, 0, sizeof(NVMECommonCommand));
        cmd.dword0 = NVME_ADMIN_SET_FEATURES;
        cmd.dword0 |= (0x9876 << 16);

        // arbitrary data pointer - the FID we will set does not actually use
        // any data transfers
        cmd.dataLow = drive->idPhys;
        cmd.dword10 = 0x19;         // FID 0x19 - set I/O command set profile
        cmd.dword11 = cmdProfile;
        cmd.dword14 = 0x00;

        nvmeSubmit(drive, 0, &cmd);
        if(!nvmePoll(drive, 0, 0x9876, 20)) {
            luxLogf(KPRINT_LEVEL_WARNING, "- timeout while setting command set profile, aborting...\n");
            return -1;
        }
    }

    // now we need to identify the NVM command set I/O namespaces
    // we will not need to iterate over anything here as this driver will only
    // implement support for one command set
    memset(&cmd, 0, sizeof(NVMECommonCommand));
    cmd.dword0 = NVME_ADMIN_IDENTIFY;
    cmd.dword0 |= (0xBEEF << 16);
    cmd.dataLow = drive->idPhys;
    cmd.dword10 = 0x07;             // CNS 0x07 = identify namespaces
    cmd.dword11 = 0;                // CSI 0x00 = NVM I/O command set
    nvmeSubmit(drive, 0, &cmd);
    if(!nvmePoll(drive, 0, 0xBEEF, 20)) {
        luxLogf(KPRINT_LEVEL_WARNING, "- timeout while identifying NVM namespaces, aborting...\n");
        return -1;
    }

    uint32_t *ns = (uint32_t *) drive->id;
    drive->nsCount = 0;
    for(int i = 0; i < 1024; i++) {
        if(ns[i] && ns[i] < 0xFFFFFFFE) drive->nsCount++;
    }

    if(!drive->nsCount) {
        luxLogf(KPRINT_LEVEL_WARNING, "- drive does not implement any namespaces, aborting...\n");
        return -1;
    }

    drive->ns = calloc(drive->nsCount, sizeof(uint32_t));
    drive->nsSectorSizes = calloc(drive->nsCount, sizeof(uint16_t));
    drive->nsSizes = calloc(drive->nsCount, sizeof(uint64_t));
    if(!drive->ns || !drive->nsSectorSizes || !drive->nsSizes) {
        luxLogf(KPRINT_LEVEL_WARNING, "- unable to allocate memory for namespaces, aborting...\n");
        return -1;
    }

    int nsIndex = 0;
    for(int i = 0; i < 1024; i++) {
        if(ns[i] && ns[i] < 0xFFFFFFFE) {
            drive->ns[nsIndex] = ns[i];
            nsIndex++;
        }
    }

    luxLogf(KPRINT_LEVEL_DEBUG, "- found %d namespace%s implementing NVM I/O commands:\n", drive->nsCount,
        drive->nsCount != 1 ? "s" : "");

    // now set up the namespaces one by one
    for(int i = 0; i < drive->nsCount; i++) {
        memset(drive->id, 0, sizeof(NVMIONSIdentification));
        memset(&cmd, 0, sizeof(NVMECommonCommand));

        // identify namespace
        cmd.dword0 = NVME_ADMIN_IDENTIFY;
        cmd.dword0 |= ((i + 0xBEEF) << 16);     // arbirary command ID
        cmd.namespaceID = drive->ns[i];
        cmd.dataLow = drive->idPhys;
        cmd.dword10 = 0x00;         // CNS 0x00 = identify namespace
        cmd.dword11 = 0x00;         // CSI 0x00 = NVM I/O command set
        nvmeSubmit(drive, 0, &cmd);
        if(!nvmePoll(drive, 0, i + 0xBEEF, 20)) {
            luxLogf(KPRINT_LEVEL_WARNING, "- timeout while identifying NVM namespace %d (0x%08X), aborting...\n", i, drive->ns[i]);
            return -1;
        }

        NVMIONSIdentification *nvmNSID = (NVMIONSIdentification *) drive->id;
        if(!nvmNSID->lbaFormatCount) {
            luxLogf(KPRINT_LEVEL_WARNING, "- drive does not report any LBA formats, aborting...\n");
            return -1;
        }

        drive->nsSectorSizes[i] = 1 << nvmNSID->lbaFormat[0].sectorSize;
        drive->nsSizes[i] = nvmNSID->capacity;
        uint64_t byteSize = drive->nsSizes[i] * drive->nsSectorSizes[i];
        if(byteSize >= 0x40000000)
            luxLogf(KPRINT_LEVEL_DEBUG, " + NS %d: capacity %d GiB, %d bytes per sector\n", i+1, byteSize/0x40000000, drive->nsSectorSizes[i]);
        else
            luxLogf(KPRINT_LEVEL_DEBUG, " + NS %d: capacity %d MiB, %d bytes per sector\n", i+1, byteSize/0x100000, drive->nsSectorSizes[i]);
        
        // register the device with the storage device layer
        // we will an internal numerical ID to identify NVMe devices where the
        // lower 16 bits refer to a namespace and the higher 16 bits refer to a
        // controller, with the highest 32 bits reserved
        SDevRegisterCommand regcmd;
        memset(&regcmd, 0, sizeof(SDevRegisterCommand));
        regcmd.header.command = COMMAND_SDEV_REGISTER;
        regcmd.header.length = sizeof(SDevRegisterCommand);
        regcmd.device = ((nvmeDriveCount() - 1) << 16) | i;
        regcmd.partitions = 1;
        regcmd.size = drive->nsSizes[i];
        regcmd.sectorSize = drive->nsSectorSizes[i];
        strcpy(regcmd.server, "lux:///dsnvme");     // server name prefixed with lux:///

        luxSendDependency(&regcmd);
    }

    // attempt to allocate a driver-specified maximum number of I/O queues
    // and then verify how many queues the drive actually allocated
    memset(&cmd, 0, sizeof(NVMECommonCommand));
    cmd.dword0 = NVME_ADMIN_SET_FEATURES;
    cmd.dword0 |= (0xC0DE << 16);      // arbitrary command ID
    cmd.dword10 = 0x07;         // FID 0x07 - allocate queues
    cmd.dword11 = ((IO_DEFAULT_QUEUE_COUNT-1) << 16) | (IO_DEFAULT_QUEUE_COUNT-1);
    nvmeSubmit(drive, 0, &cmd);

    NVMECompletionQueue *res = nvmePoll(drive, 0, 0xC0DE, 20);
    if(!res) {
        luxLogf(KPRINT_LEVEL_DEBUG, "- timeout while allocating I/O queues, aborting...\n");
        return -1;
    }

    drive->cqCount = (res->dword0 >> 16) + 1;
    drive->sqCount = (res->dword0 & 0xFFFF) + 1;

    // ensure we have a 1:1 correspondence by using only the smallest value
    if(drive->cqCount > IO_DEFAULT_QUEUE_COUNT)
        drive->cqCount = IO_DEFAULT_QUEUE_COUNT;
    
    if(drive->sqCount > IO_DEFAULT_QUEUE_COUNT)
        drive->sqCount = IO_DEFAULT_QUEUE_COUNT;

    if(drive->cqCount != drive->sqCount) {
        luxLogf(KPRINT_LEVEL_WARNING, "- %d submission queues, %d completion queues; using smaller value\n",
            drive->sqCount, drive->cqCount);
        if(drive->cqCount < drive->sqCount) drive->sqCount = drive->cqCount;
        else drive->cqCount = drive->sqCount;
    } else {
        luxLogf(KPRINT_LEVEL_DEBUG, "- %d submission queues, %d completion queues\n", drive->sqCount, drive->cqCount);
    }

    // now allocate memory for the queue pointer arrays
    drive->ioTails = calloc(drive->sqCount, sizeof(int));
    drive->ioHeads = calloc(drive->sqCount, sizeof(int));

    // and queue busy status
    drive->ioBusy = calloc(drive->sqCount, sizeof(int));

    if(!drive->ioTails || !drive->ioHeads || !drive->ioBusy) {
        luxLogf(KPRINT_LEVEL_WARNING, "- unable to allocate memory for I/O queues heads and tails\n");
        return -1;
    }

    drive->sqPhys = calloc(drive->sqCount, sizeof(uintptr_t));
    drive->cqPhys = calloc(drive->cqCount, sizeof(uintptr_t));
    drive->sq = calloc(drive->sqCount, sizeof(NVMECommonCommand *));
    drive->cq = calloc(drive->cqCount, sizeof(NVMECompletionQueue *));

    if(!drive->sqPhys || !drive->cqPhys || !drive->sq || !drive->cq) {
        luxLogf(KPRINT_LEVEL_WARNING, "- unable to allocate memory for I/O queue pointer array\n");
        return -1;
    }

    // determine the maximum size of the queue support by the device
    drive->ioQSize = (cap & NVME_CAP_MAXQ_MASK) + 1;
    if(drive->ioQSize > IO_DEFAULT_QUEUE_SIZE)
        drive->ioQSize = IO_DEFAULT_QUEUE_SIZE;

    luxLogf(KPRINT_LEVEL_DEBUG, "- maximum %d commands per I/O queue\n", drive->ioQSize);

    // set up the controller's queue entry sizes
    uint32_t config = nvmeRead32(drive, NVME_CONFIG);
    config &= ~(NVME_CONFIG_SQES_MASK << NVME_CONFIG_SQES_SHIFT);
    config &= ~(NVME_CONFIG_CQES_MASK << NVME_CONFIG_CQES_SHIFT);
    config |= (6 << NVME_CONFIG_SQES_SHIFT);        // 64 bytes
    config |= (4 << NVME_CONFIG_CQES_SHIFT);        // 16 bytes
    nvmeWrite32(drive, NVME_CONFIG, config);

    // and allocate contiguous memory for each
    for(int i = 0; i < drive->sqCount; i++) {
        drive->sqPhys[i] = pcontig(0, sizeof(NVMECommonCommand) * drive->ioQSize, 0);
        drive->cqPhys[i] = pcontig(0, sizeof(NVMECompletionQueue) * drive->ioQSize, 0);
        if(!drive->sqPhys[i] || !drive->cqPhys[i]) {
            luxLogf(KPRINT_LEVEL_WARNING, "- unable to allocate memory for I/O queues %d\n", i);
            return -1;
        }

        // map the queues to virtual memory
        drive->sq[i] = (NVMECommonCommand *) mmio(drive->sqPhys[i],
            sizeof(NVMECommonCommand) * drive->ioQSize, MMIO_R | MMIO_W | MMIO_CD | MMIO_ENABLE);
        drive->cq[i] = (NVMECompletionQueue *) mmio(drive->cqPhys[i],
            sizeof(NVMECompletionQueue) * drive->ioQSize, MMIO_R | MMIO_W | MMIO_CD | MMIO_ENABLE);

        if(!drive->sq[i] || !drive->cq[i]) {
            luxLogf(KPRINT_LEVEL_WARNING, "- unable to map I/O queues %d to virtual memory\n", i);
            return -1;
        }

        // request queue creation from the drive
        // completion queues
        memset(&cmd, 0, sizeof(NVMECommonCommand));
        cmd.dword0 = NVME_ADMIN_CREATE_COMQ;
        cmd.dword0 |= (i + 0x1234) << 16;
        cmd.dataLow = drive->cqPhys[i];         // PRP1
        cmd.dword10 = ((drive->ioQSize-1) << 16) | (i+1);   // queue size and ID

        // TODO: enable interrupts here after implementing MSI/MSI-X
        cmd.dword11 = 0x01;     // interrupts disabled, physically contiguous
        nvmeSubmit(drive, 0, &cmd);
        res = nvmePoll(drive, 0, (i + 0x1234), 20);
        
        if(!res) {
            luxLogf(KPRINT_LEVEL_WARNING, "- timeout while creating completion queue %d, aborting...\n", i);
            return -1;
        }

        // submission queues next
        memset(&cmd, 0, sizeof(NVMECommonCommand));
        cmd.dword0 = NVME_ADMIN_CREATE_SUBQ;
        cmd.dword0 |= (i + 0xABCD) << 16;       // arbitrary command ID
        cmd.dataLow = drive->sqPhys[i];         // PRP1
        cmd.dword10 = ((drive->ioQSize-1) << 16) | (i+1);   // submission queue identifier
        cmd.dword11 = ((i+1) << 16) | 0x01;     // 1:1 mapping with completion queue; physically contiguous
        cmd.dword12 = 0;    // not associated with a specific command set
        nvmeSubmit(drive, 0, &cmd);
        res = nvmePoll(drive, 0, (i + 0xABCD), 20);
        
        if(!res) {
            luxLogf(KPRINT_LEVEL_WARNING, "- timeout while creating submission queue %d, aborting...\n", i);
            return -1;
        }
    }

    // at this point the controller is ready
    return 0;
}