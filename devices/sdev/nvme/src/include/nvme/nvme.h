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

    int doorbellStride;
    int maxQueues;
    int minPage, maxPage;
} NVMEController;

/* NVMe Common Command Format: these are the entries that are submitted to the
 * admin and I/O submission queues, where the number of entries is dictated by
 * the controller's capability register */

typedef struct {
    uint32_t dword0;
    uint32_t namespaceID;
    uint32_t dword2;
    uint32_t dword3;
    uint64_t metaptr;   // metadata pointer
    uint64_t dataLow;   // data pointer
    uint64_t dataHigh;
    uint32_t dword10;
    uint32_t dword11;
    uint32_t dword12;
    uint32_t dword13;
    uint32_t dword14;
    uint32_t dword15;
}__attribute__((packed)) NVMECommonCommand;

/* The lowest 8 bits of DWORD0 contain the command opcode and the direction of
 * the data transfer, if any. The higher 16 bits also contain a unique ID
 * number for this command, that can be used in conjunction with the submission
 * queue identifier in the NVMe Completion Queue Entry defined below to identify
 * which command was completed. */

/* NVMe Common Completion Queue Entry Format */

typedef struct {
    uint32_t dword0;
    uint32_t dword1;
    uint16_t sqHeadPointer;
    uint16_t sqIdentifier;
    uint16_t commandID;
    uint16_t status;
}__attribute__((packed)) NVMeCompletionQueue;

int nvmeInit(const char *);
