/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * sdev: Abstraction for storage devices under /dev/sdX
 */

#pragma once

#include <sys/types.h>
#include <liblux/liblux.h>
#include <liblux/sdev.h>

typedef struct StorageDevice {
    struct StorageDevice *next;
    char name[256];             // name under /dev
    char server[256];           // server that handles this device
    uint64_t deviceID;          // driver-specific ID for this device
    int partition;              // partitioned?
    uint64_t size;              // sectors
    uint16_t sectorSize;        // bytes

    int root;                   // set for the root of a partitioned device

    // the remaining fields are not valid if root == 0
    uint64_t partitionStart;    // sectors
    uint64_t partitionSize;     // sectors
} StorageDevice;

extern int drvCount, devCount;
extern StorageDevice *sdev;

void registerDevice(SDevRegisterCommand *);
StorageDevice *findDevice(int);
void sdevRead(RWCommand *);