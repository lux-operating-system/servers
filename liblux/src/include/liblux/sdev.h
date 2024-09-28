/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * liblux: Library abstracting kernel-server communication protocols
 */

#pragma once

#include <liblux/liblux.h>

/* These commands are requested by storage device drivers from sdev */
#define COMMAND_SDEV_REGISTER           0xE001
#define COMMAND_SDEV_UNREGISTER         0xE002
#define COMMAND_SDEV_READ               0xE003
#define COMMAND_SDEV_WRITE              0xE004

#define COMMAND_MIN_SDEV                0xE001
#define COMMAND_MAX_SDEV                0xE004

typedef struct {
    MessageHeader header;
    char server[256];           // driver to handle the request
    uint64_t device;            // driver-specific ID for the storage device
    uint64_t size;              // sectors
    uint16_t sectorSize;        // bytes
    int partitions;             // 1 if the device is partitioned, 0 if not
} SDevRegisterCommand;

typedef struct {
    MessageHeader header;
    uint16_t syscall;           // corresponding syscall ID

    uint64_t device;            // driver-specific device ID
    uint64_t start;             // sectors
    uint64_t count;             // sectors

    uint64_t buffer[];
} SDevRWCommand;
