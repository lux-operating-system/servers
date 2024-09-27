/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * nvme: Device driver for NVMe SSDs
 */

#pragma once

#include <sys/types.h>

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

typedef struct NVMEController {
    struct NVMEController *next;
    char addr[16];
    uint64_t base, size;
    void *regs;     // MMIO

    int doorbellStride;
    int maxQueueEntries;
    int minPage, maxPage;
    int pageSize;

    // admin queues
    uintptr_t asqPhys, acqPhys;
    NVMECommonCommand *asq;
    NVMeCompletionQueue *acq;

    // I/O queues
    uintptr_t *sqPhys, *cqPhys;
    NVMECommonCommand **sq;
    NVMeCompletionQueue **cq;
} NVMEController;

/* Data Structure Returned by the Admin Identify Command */

typedef struct {
    // Controller Capabilities and Features
    uint16_t pciVendor;
    uint16_t pciSubvendor;
    uint8_t serial[20];
    uint8_t model[40];
    uint64_t firmwareRevision;
    uint8_t arbitrationBurst;
    uint8_t ouiIdentifier[3];
    uint8_t controllerMultipath;    // multiple controller capabilities
    uint8_t maximumTransfer;        // (2^n) * minimum page size
    uint16_t controllerID;
    uint32_t version;
    uint32_t rtd3ResumeLatency;
    uint32_t rtd3EntryLatency;
    uint32_t asyncEventCapabilities;
    uint32_t controllerAttributes;
    uint16_t readRecovery;
    uint8_t bootCapabilities;
    uint8_t reserved1;
    uint32_t shutdownLatency;
    uint16_t reserved2;
    uint8_t powerLossSignals;
    uint8_t controllerType;
    uint8_t fguid[16];
    uint16_t commandRetryDelay[3];
    uint8_t reachabilityCapabilities;
    uint8_t reserved3[118];
    uint8_t subsystemReport;
    uint8_t vpdWriteCycle;
    uint8_t mgmtEndpointCapabilities;

    // Admin Command Set Identification
    uint16_t optionalAdminCommands;
    uint8_t abortCommandLimit;
    uint8_t asyncEventLimit;
    uint8_t firmwareUpdates;
    uint8_t logPageAttributes;
    uint8_t errorLogEntries;
    uint8_t powerStateSupport;
    uint8_t vendorSpecificCommand;
    uint8_t autonomousPowerMgmt;
    uint16_t warningTemperature;
    uint16_t criticalTemperature;
    uint16_t maxFirmwareActivationTime;
    uint32_t hostMemoryPreferredSize;
    uint32_t hostMemoryMinimumSize;
    uint64_t totalNVMCapacity[2];       // bytes
    uint64_t unallocatedNVMCapacity[2]; // bytes
    uint32_t replayBlockSupport;
    uint16_t extendedSelfTestTime;
    uint8_t selfTestOptions;
    uint8_t firmwareUpdateGranularity;
    uint16_t keepAliveSupport;
    uint16_t thermalAttributes;
    uint16_t minimumTemp;
    uint16_t maximumTemp;
    uint32_t sanitizeCapabilities;
    uint32_t minHostBufDescriptorSize;
    uint16_t maxHostMemDescriptors;
    uint16_t setIdentifierMaximum;
    uint16_t enduranceMaximum;
    uint8_t anaTransitionTime;
    uint8_t assymmetricNamespaceCapabilities;
    uint32_t anaGroupMaximum;
    uint32_t anaGroupIDs;
    uint32_t persistentLogSize;
    uint16_t domainID;
    uint8_t keyPerIOCapabilities;
    uint8_t reserved4;
    uint16_t maxFirmwareActivationTimeWR;
    uint8_t reserved5[6];
    uint8_t maxEndurance[16];
    uint8_t tempAttributes;
    uint8_t reserved6;
    uint16_t commandQuiesceTime;
    uint8_t reserved7[124];

    // NVM Command Set Identification
    uint8_t minmaxSQ;           // I/O submission queue
    uint8_t minmaxCQ;           // I/O completion queue
    uint16_t maxcmd;            // maximum commands per queue
    uint32_t namespaceCount;
    uint16_t optionalNVMCommands;
    uint16_t fusedSupport;
    uint8_t formatNVMAttributes;
    uint8_t volatileWriteCache;
    uint16_t atomicWriteNormal;
    uint16_t atomicWriteFail;
    uint8_t vendorSpecificConfiguration;
    uint8_t namespaceWP;        // write-protection
    uint16_t atomicCmpWU;       // atomic compare and write unit
    uint16_t copyDescriptors;
    uint32_t sglSupport;
    uint32_t maxAllowedNamespaces;
    uint8_t maxNamespaceAttachments[16];
    uint32_t maxIONamespaces;
    uint32_t optimalQueueDepth;
    uint8_t refreshInterval;
    uint8_t refreshTime;
    uint16_t controllerMaxMemoryTrackingDescriptors;
    uint16_t nvmMaxMemoryTrackingDescriptors;
    uint8_t minMemoryRangeTrackingGranularity;
    uint8_t maxMemoryRangeTrackingGranularity;
    uint8_t trackingAttributes;
    uint8_t reserved8;
    uint16_t maxUserMigrationQeueus;
    uint16_t nvmMaxUserMigrationQueues;
    uint16_t maxCDQ;
    uint16_t nvmMaxCDQ;
    uint16_t maxPRPCount;
    uint8_t reserved9[180];
    uint8_t qualifiedName[256];
    uint8_t reserved10[768];

    // Fabric-Specific Region
    // we will not implement a driver for this so we won't define its structure
    uint8_t fabric[1280];

    // Vendor-Specific Region
    uint8_t vendor[1024];
}__attribute__((packed)) NVMEIdentification;

/* Controller Type */
#define NVME_CONTROLLER_IO          1
#define NVME_CONTROLLER_DISCOVERY   2
#define NVME_CONTROLLER_ADMIN       3

int s = sizeof(NVMEIdentification);

int nvmeInit(const char *);
int nvmeIdentify(NVMEController *);
