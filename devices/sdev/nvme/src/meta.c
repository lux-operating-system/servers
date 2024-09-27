/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * nvme: Device driver for NVMe SSDs
 */

#include <nvme/nvme.h>
#include <nvme/registers.h>
#include <liblux/liblux.h>
#include <sys/lux/lux.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static NVMEController *devices = NULL;
static int deviceCount = 0;

/* nvmeAllocateDrive(): allocates an NVMe drive
 * params: none
 * returns: pointer to the allocated device struct, NULL on error
 */

NVMEController *nvmeAllocateDrive() {
    NVMEController *dev = calloc(1, sizeof(NVMEController));
    if(!dev) return NULL;

    deviceCount++;

    if(!devices) {
        devices = dev;
        return dev;
    }

    NVMEController *list = devices;
    while(list->next)
        list = list->next;

    list->next = dev;
    return dev;
}

/* nvmeInit(): detects and initializes an NVMe controller
 * params: addr - PCI address in the format of "BB.SS.FF"
 * returns: zero on success
 */

int nvmeInit(const char *addr) {
    char path[32];

    // read BAR0 and create an MMIO mapping
    uint64_t bar0, bar0size;
    sprintf(path, "/dev/pci/%s/bar0", addr);

    FILE *file = fopen(path, "rb");
    if(!file) return -1;

    size_t s = fread(&bar0, 1, 8, file);
    fclose(file);
    if(s != 8) return -1;

    sprintf(path, "/dev/pci/%s/bar0size", addr);
    file = fopen(path, "rb");
    if(!file) return -1;

    s = fread(&bar0size, 1, 8, file);
    fclose(file);
    if(s != 8) return -1;

    // create MMIO
    void *ptr = (void *) mmio(bar0, bar0size, MMIO_R | MMIO_W | MMIO_CD | MMIO_ENABLE);
    if(!ptr) return -1;

    luxLogf(KPRINT_LEVEL_DEBUG, "- base memory @ [0x%X - 0x%X]\n", bar0, bar0+bar0size-1);

    NVMEController tempdrv;
    tempdrv.regs = ptr;

    uint64_t cap = nvmeRead64(&tempdrv, NVME_CAP);
    char capstr[256];
    sprintf(capstr, "%s, %s%s%s",
        cap & NVME_CAP_CONT_QUEUE ? "contiguous queues" : "fragmented queues",
        cap & NVME_CAP_RESET ? "reset, " : "",
        cap & NVME_CAP_NVM_CMDS ? "NVM commands, " : "",
        cap & NVME_CAP_IO_CMDS ? "I/O commands" : "");
    
    luxLogf(KPRINT_LEVEL_DEBUG, "- capability: 0x%X (%s)\n", cap, capstr);

    if(!(cap & NVME_CAP_NVM_CMDS)) {
        luxLogf(KPRINT_LEVEL_WARNING, "- drive does not support NVM command set, aborting\n");
        return -1;
    }

    NVMEController *drive = nvmeAllocateDrive();
    if(!drive) {
        luxLogf(KPRINT_LEVEL_WARNING, "- unable to allocate memory for drive\n");
        return -1;
    }

    strcpy(drive->addr, addr);
    drive->base = bar0;
    drive->size = bar0size;
    drive->regs = ptr;

    drive->maxQueues = (cap & NVME_CAP_MAXQ_MASK) + 1;
    drive->doorbellStride = 4 << ((cap & NVME_CAP_DSTRD_MASK) >> NVME_CAP_DSTRD_SHIFT);
    drive->maxPage = 1 << (((cap & NVME_CAP_MPSMAX_MASK) >> NVME_CAP_MPSMAX_SHIFT) + 12);
    drive->minPage = 1 << (((cap & NVME_CAP_MPSMIN_MASK) >> NVME_CAP_MPSMIN_SHIFT) + 12);
    
    luxLogf(KPRINT_LEVEL_DEBUG, "- max %d queues, doorbell stride %d\n", drive->maxQueues, drive->doorbellStride);
    luxLogf(KPRINT_LEVEL_DEBUG, "- valid page sizes range from %d KiB - %d KiB\n", drive->minPage/1024, drive->maxPage/1024);

    // disable the device
    nvmeWrite32(drive, NVME_CONFIG, nvmeRead32(drive, NVME_CONFIG) & ~NVME_CONFIG_EN);

    // configure the device to use larger pages if supported
    uint32_t cc = nvmeRead32(drive, NVME_CONFIG);
    cc &= ~(NVME_CONFIG_MPS_MASK << NVME_CONFIG_MPS_SHIFT);
    if(drive->maxPage >= 65536) {
        cc |= (4 << NVME_CONFIG_MPS_SHIFT);
        drive->pageSize = 65536;
    } else if(drive->maxPage >= 32768) {
        cc |= (3 << NVME_CONFIG_MPS_SHIFT);
        drive->pageSize = 32768;
    } else if(drive->maxPage >= 16384) {
        cc |= (2 << NVME_CONFIG_MPS_SHIFT);
        drive->pageSize = 16384;
    } else {
        drive->pageSize = 4096;
    }

    luxLogf(KPRINT_LEVEL_DEBUG, "- set page size to %d KiB\n", drive->pageSize/1024);
    
    // enable the NVM command set
    cc &= ~(NVME_CONFIG_CMDS_MASK << NVME_CONFIG_CMDS_SHIFT);
    cc |= (NVME_CONFIG_CMDS_NVM << NVME_CONFIG_CMDS_SHIFT);

    // I/O submission queue entries will be set to 64 bytes, and completion
    // queue entries will be 16 bytes
    cc &= ~(NVME_CONFIG_SQES_MASK << NVME_CONFIG_SQES_SHIFT);
    cc |= (7 << NVME_CONFIG_SQES_SHIFT);

    cc &= ~(NVME_CONFIG_CQES_MASK << NVME_CONFIG_CQES_SHIFT);
    cc |= (4 << NVME_CONFIG_CQES_SHIFT);
    nvmeWrite32(drive, NVME_CONFIG, cc);

    // set up the admin queue sizes with 64 entries in each of the submission
    // and completion queues
    uint32_t aqa = (64 << 16) | 64;
    nvmeWrite32(drive, NVME_AQA, aqa);

    // allocate memory for the admin queues
    drive->asqPhys = pcontig(0, sizeof(NVMECommonCommand) * 64, 0);
    drive->acqPhys = pcontig(0, sizeof(NVMeCompletionQueue) * 64, 0);

    if(!drive->asqPhys || !drive->acqPhys) {
        luxLogf(KPRINT_LEVEL_WARNING, "- unable to allocate memory for admin queues\n");
        return -1;
    }

    luxLogf(KPRINT_LEVEL_DEBUG, "- admin queues at 0x%X, 0x%X\n", drive->asqPhys, drive->acqPhys);

    // map the queues
    drive->asq = (NVMECommonCommand *) mmio(drive->asqPhys, sizeof(NVMECommonCommand) * 64, MMIO_R | MMIO_W | MMIO_CD | MMIO_ENABLE);
    drive->acq = (NVMeCompletionQueue *) mmio(drive->asqPhys, sizeof(NVMeCompletionQueue) * 64, MMIO_R | MMIO_W | MMIO_CD | MMIO_ENABLE);

    if(!drive->asq || !drive->acq) {
        luxLogf(KPRINT_LEVEL_WARNING, "- unable to memory map admin queues\n");
        pcontig(drive->asqPhys, sizeof(NVMECommonCommand) * 64, 0);
        pcontig(drive->acqPhys, sizeof(NVMeCompletionQueue) * 64, 0);
        return -1;
    }

    // reset to zero
    memset(drive->asq, 0, sizeof(NVMECommonCommand)*64);
    memset(drive->acq, 0, sizeof(NVMeCompletionQueue)*64);

    // and give them to the device
    nvmeWrite64(drive, NVME_ASQ, drive->asqPhys);
    nvmeWrite64(drive, NVME_ACQ, drive->acqPhys);

    // now enable the device
    nvmeWrite32(drive, NVME_CONFIG, nvmeRead32(drive, NVME_CONFIG) | NVME_CONFIG_EN);
    while(!(nvmeRead32(drive, NVME_STATUS) & NVME_STATUS_RDY));

    return 0;
}