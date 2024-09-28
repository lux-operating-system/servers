/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * nvme: Device driver for NVMe SSDs
 */

#pragma once

#include <sys/types.h>
#include <liblux/sdev.h>

/* This queue-like linked list structure will be used to track I/O requests
 * made via the storage device abstraction layer */

typedef struct IORequest {
    struct IORequest *next;
    int drive, ns;
    int queue;      // which queue the command was submitted to
    int timeout;    // number of times to cycle before timing out
    uint16_t id;    // unique non-zero syscall ID and NVMe command ID

    // this will be relayed back to the sdev server
    SDevRWCommand *rwcmd;

    // this is for freeing memory if necessary
    int pageBoundaries;
    uint64_t prp2;
} IORequest;
