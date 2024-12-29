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

typedef struct {
    uint8_t flags;
    uint8_t chsStart[3];
    uint8_t id;
    uint8_t chsEnd[3];
    uint32_t start;
    uint32_t size;
}__attribute__((packed)) MBRPartition;

typedef struct StorageDevice {
    struct StorageDevice *next;
    char name[256];             // name under /dev
    char server[256];           // server that handles this device
    uint64_t deviceID;          // driver-specific ID for this device
    int partition;              // partitioned?
    uint64_t size;              // sectors
    uint16_t sectorSize;        // bytes

    int sd;                     // server socket

    int partitionCount;
    uint64_t partitionStart[16];    // sectors
    uint64_t partitionSize[16];     // sectors
} StorageDevice;

extern int drvCount, devCount;
extern StorageDevice *sdev;

void registerDevice(int, SDevRegisterCommand *);
void partitionDevice(const char *, StorageDevice *);
StorageDevice *findDevice(int);
void sdevRead(RWCommand *);
void relayRead(SDevRWCommand *);
void sdevWrite(RWCommand *);
void relayWrite(SDevRWCommand *);
