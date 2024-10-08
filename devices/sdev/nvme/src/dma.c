/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * nvme: Device driver for NVMe SSDs
 */

/* Implementation of Direct Memory Access for NVMe via PRPs */

#include <nvme/nvme.h>
#include <sys/lux/lux.h>

/* nvmeCreatePRP(): creates a physical region page for an NVMe command
 * note: the offset into the page must be DWORD-aligned (i.e. bits 0 and 1 of
 * all physical addresses must always be cleared)
 * params: drive - drive to execute this command
 * params: cmd - common command structure
 * params: data - data pointer in virtual memory
 * params: len - size of the expected data transfer
 * returns: non-negative page boundary count on success, negative on fail
 */

int nvmeCreatePRP(NVMEController *drive, NVMECommonCommand *cmd, const void *data, size_t len) {
    // count how many pages this will use
    size_t pageCount = (len + drive->pageSize - 1) / drive->pageSize;
    if(!pageCount) return -1;

    // and how many page boundaries will be crossed
    off_t pageOffset = (uintptr_t) data & ~drive->pageSize;
    size_t pageBoundaries = pageCount - 1;
    if(pageOffset) pageBoundaries++;

    if(!pageBoundaries) {
        // special case for zero page boundaries crossed: PRP1 is the physical
        // address of the first page, and PRP2 is reserved
        cmd->dataLow = vtop((uintptr_t)data);
        cmd->dataHigh = 0;
        if(!cmd->dataLow || (cmd->dataLow & 3)) return -1;
        return 0;
    }

    if(pageBoundaries == 1) {
        // special case for exactly one page boundary crossed: PRP1 is the
        // physical address of the first page and PRP2 is the physical address
        // of the second page with its offset cleared to zero
        cmd->dataLow = vtop((uintptr_t)data);
        cmd->dataHigh = vtop((uintptr_t)data + drive->pageSize) & ~(drive->pageSize-1);

        if(!cmd->dataLow || !cmd->dataHigh || (cmd->dataLow & 3)) return -1;
        return 1;
    }

    // for all other cases, PRP1 is the physical address of the first page and
    // PRP2 is the physical address of a contiguous PRP table containing all
    // other physical addresses of all remaining pages
    cmd->dataLow = vtop((uintptr_t)data);
    if(!cmd->dataLow || (cmd->dataLow & 3)) return -1;

    cmd->dataHigh = pcontig(0, (pageCount-1) * drive->pageSize, 0);
    if(!cmd->dataHigh) return -1;

    uint64_t *prpTable = (uint64_t *) mmio(cmd->dataHigh, (pageCount-1) * drive->pageSize,
        MMIO_R | MMIO_W | MMIO_CD | MMIO_ENABLE);
    
    if(!prpTable) {
        pcontig(cmd->dataHigh, (pageCount-1) * drive->pageSize, 0);
        return -1;
    }

    // now populate the PRP table
    for(int i = 1; i < pageBoundaries; i++) {
        prpTable[i-1] = vtop((uintptr_t)data + (i * drive->pageSize));
        prpTable[i-1] &= ~(drive->pageSize-1);
        if(!prpTable[i-1]) {
            mmio((uintptr_t)prpTable, (pageCount-1) * drive->pageSize, 0);     // delete mapping
            pcontig(cmd->dataHigh, (pageCount-1) * drive->pageSize, 0);
            return -1;
        }
    }

    return pageBoundaries;
}

/* nvmeDestroyPRP(): destroys a physical region page of an NVMe command
 * params: drive - drive that executed the command
 * params: prp2 - physical region page 2 (NVMe Base Spec, figure 92)
 * params: len - size of the data transfer
 * returns: zero on success
 */

int nvmeDestroyPRP(NVMEController *drive, uint64_t prp2, size_t len) {
    return 0;   // TODO
}