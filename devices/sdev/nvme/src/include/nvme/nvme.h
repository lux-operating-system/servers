/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * nvme: Device driver for NVMe SSDs
 */

#pragma once

#include <sys/types.h>

typedef struct NVMEController {
    struct NVMEController *next;
    char addr[16];
    uint64_t base, size;
    void *regs;     // MMIO
} NVMEController;

int nvmeInit(const char *);
