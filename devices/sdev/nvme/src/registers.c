/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * nvme: Device driver for NVMe SSDs
 */

#include <nvme/nvme.h>

/* nvmeRead32(): reads a 32-bit NVMe register
 * params: drive - NVMe drive structure
 * params: offset - register offset
 * returns: value
 */

uint32_t nvmeRead32(NVMEController *drive, off_t offset) {
    uint32_t volatile *ptr = (uint32_t volatile *)((uintptr_t) drive->regs + offset);
    return *ptr;
}

/* nvmeRead64(): reads a 64-bit NVMe register
 * params: drive - NVMe drive structure
 * params: offset - register offset
 * returns: value
 */

uint64_t nvmeRead64(NVMEController *drive, off_t offset) {
    uint64_t volatile *ptr = (uint64_t volatile *)((uintptr_t) drive->regs + offset);
    return *ptr;
}

/* nvmeWrite32(): writes a 32-bit NVMe register
 * params: drive - NVMe drive structure
 * params: offset - register offset
 * params: data - value to write
 * returns: nothing
 */

void nvmeWrite32(NVMEController *drive, off_t offset, uint32_t data) {
    uint32_t volatile *ptr = (uint32_t volatile *)((uintptr_t) drive->regs + offset);
    *ptr = data;
}

/* nvmeWrite64(): writes a 64-bit NVMe register
 * params: drive - NVMe drive structure
 * params: offset - register offset
 * params: data - value to write
 * returns: nothing
 */

void nvmeWrite64(NVMEController *drive, off_t offset, uint64_t data) {
    uint64_t volatile *ptr = (uint64_t volatile *)((uintptr_t) drive->regs + offset);
    *ptr = data;
}
