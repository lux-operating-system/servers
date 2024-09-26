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

    luxLogf(KPRINT_LEVEL_DEBUG, " - base memory @ 0x%X - 0x%X\n", bar0, bar0+bar0size-1);

    NVMEController tempdrv;
    tempdrv.regs = ptr;

    uint64_t cap = nvmeRead64(&tempdrv, NVME_CAP);
    char capstr[256];
    sprintf(capstr, "%s, %s%s%s",
        cap & NVME_CAP_CONT_QUEUE ? "contiguous queues" : "fragmented queues",
        cap & NVME_CAP_RESET ? "reset, " : "",
        cap & NVME_CAP_NVM_CMDS ? "NVM commands, " : "",
        cap & NVME_CAP_IO_CMDS ? "I/O commands" : "");
    
    luxLogf(KPRINT_LEVEL_DEBUG, " - capabilities: 0x%X (%s)\n", cap, capstr);

    if(!(cap & NVME_CAP_NVM_CMDS)) {
        luxLogf(KPRINT_LEVEL_WARNING, " - drive does not support NVM command set, aborting\n");
        return -1;
    }

    NVMEController *drive = nvmeAllocateDrive();
    if(!drive) {
        luxLogf(KPRINT_LEVEL_WARNING, " - unable to allocate memory for drive\n");
        return -1;
    }

    strcpy(drive->addr, addr);
    drive->base = bar0;
    drive->size = bar0size;
    drive->regs = ptr;

    return 0;
}