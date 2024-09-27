/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * nvme: Device driver for NVMe SSDs
 */

/* Implementation of NVMe Admin Commands */

#include <liblux/liblux.h>
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
    cmd.dword10 = 0x01;             // CNS 1 = identify controller

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

    luxLogf(KPRINT_LEVEL_DEBUG, "- serial number: %s\n", drive->serial);
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
    if(!drive->ns) {
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

    luxLogf(KPRINT_LEVEL_DEBUG, "- found %d namespace%s implementing NVM I/O commands\n", drive->nsCount,
        drive->nsCount != 1 ? "s" : "");

    while(1);
}