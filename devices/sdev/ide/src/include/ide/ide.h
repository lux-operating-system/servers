/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * ide: Device driver for IDE (ATA HDDs)
 */

#pragma once

#include <sys/types.h>

/* default base I/O ports */
#define ATA_PRIMARY_BASE            0x01F0
#define ATA_PRIMARY_STATUS          0x03F6
#define ATA_SECONDARY_BASE          0x0170
#define ATA_SECONDARY_STATUS        0x0376

#define ATA_SECTOR_COUNT            0x02
#define ATA_LBA_LOW                 0x03
#define ATA_LBA_MID                 0x04
#define ATA_LBA_HIGH                0x05
#define ATA_DRIVE_SELECT            0x06
#define ATA_COMMAND_STATUS          0x07

/* ATA command set */
#define ATA_IDENTIFY                0xEC
#define ATA_READ28                  0x20
#define ATA_READ48                  0x24
#define ATA_WRITE28                 0x30
#define ATA_WRITE48                 0x34
#define ATA_FLUSH28                 0xE7
#define ATA_FLUSH48                 0xEA

/* ATA status bits */
#define ATA_STATUS_ERROR            0x01
#define ATA_STATUS_DATA_REQUEST     0x08
#define ATA_STATUS_DRIVE_FAULT      0x20

/* ATA identify data */
#define ATA_CONFIG1_ATA             0x8000
#define ATA_CONFIG1_INCOMPLETE      0x0004

#define ATA_MAX_DATA_SIZE_MASK      0x00FF

#define ATA_CAP1_DMA                0x0100
#define ATA_CAP1_LBA                0x0200
#define ATA_CAP1_IORDY_DISABLED     0x0400
#define ATA_CAP1_IORDY              0x0800

#define ATA_IOCAP_MULTIPLE_LOGICAL  0x0100
#define ATA_IOCAP_SANITIZE          0x0400
#define ATA_IOCAP_OVERWRITE         0x4000
#define ATA_IOCAP_BLOCK_ERASE       0x8000

#define ATA_DMACAP_MODE_0           0x0001
#define ATA_DMACAP_MODE_1           0x0002
#define ATA_DMACAP_MODE_2           0x0004
#define ATA_DMACAP_MODE_0_ACTIVE    0x0100
#define ATA_DMACAP_MODE_1_ACTIVE    0x0200
#define ATA_DMACAP_MODE_2_ACTIVE    0x0400

#define ATA_PIOCAP_MODE_3           0x0001
#define ATA_PIOCAP_MODE_4           0x0002

#define ATA_CAP3_NV_WRITES          0x0004  /* non-volatile write cache */
#define ATA_CAP3_LBA28              0x0040

#define ATA_CMDCAP2_LBA48           0x0400  /* not sure why this duplication is necessary but */
#define ATA_CMDCAP5_LBA48           0x0400  /* the spec says so lol */

#define ATA_PSS_MULTIPLE            0x2000  /* multiple logical sectors per physical sector */
#define ATA_PSS_LARGE_LOGICAL       0x1000  /* logical sector size > 512 bytes */
#define ATA_PSS_MASK                0x0007

#define ATA_TMR_SERIAL              0x1000  /* transport major revision */
#define ATA_TMR_MASK                0x0003
#define ATA_TMR_ATA                 0x0001
#define ATA_TMR_ATAPI               0x0002

typedef struct {
    uint16_t config1;
    uint16_t ob1;           // obsolete
    uint16_t config2;
    uint16_t ob2[4];
    uint32_t reserved1;
    uint16_t ob3;
    uint8_t serial[20];
    uint32_t ob4;
    uint16_t ob5;
    uint8_t firmware[8];
    uint8_t model[40];
    uint16_t maxDataSize;
    uint16_t trustedFeatures;
    uint16_t cap1;
    uint16_t cap2;
    uint16_t ob6[2];
    uint16_t config3;
    uint16_t ob7[5];
    uint16_t ioCap;
    uint32_t logicalSize28; // 28-bit command set
    uint16_t ob8;
    uint16_t DMACap;
    uint16_t PIOCap;
    uint16_t DMATimePerWord1;
    uint16_t DMATimePerWord2;
    uint16_t minPIOTime;
    uint16_t minPIOTimeWithIORDY;
    uint16_t cap3;
    uint16_t reserved2;
    uint16_t reservedATAPI[4];
    uint16_t queueDepth;
    uint16_t SATACap1;
    uint16_t SATACap2;
    uint16_t SATAFeaturesCap;
    uint16_t SATAFeaturesEn;
    uint16_t majorRevision;
    uint16_t minorRevision;
    uint16_t cmdCap1;
    uint16_t cmdCap2;
    uint16_t cmdCap3;
    uint16_t cmdCap4;
    uint16_t cmdCap5;
    uint16_t cmdCap6;
    uint16_t ultraDMACap;
    uint16_t extTime1;
    uint16_t extTime2;
    uint16_t APMLevel;
    uint16_t masterPassword;
    uint16_t reset;
    uint16_t ob9;
    uint16_t minStreamSize;
    uint16_t streamTimeDMA;
    uint16_t streamLatency;
    uint32_t streamGranularity;
    uint64_t logicalSize48; // 48-bit command set
    uint16_t streamTimePIO;
    uint16_t datasetMgmtMax;
    uint16_t physicalSectorSize;
    uint16_t seekDelay;
    uint16_t wwName[4];
    uint16_t reserved3[4];
    uint16_t ob10;
    uint32_t logicalSectorSize;
    uint16_t cmdCap7;
    uint16_t cmdCap8;
    uint16_t reserved4[6];
    uint16_t ob11;
    uint16_t security;
    uint16_t vendor1[31];
    uint16_t reserved5[8];
    uint16_t deviceNominalFF;
    uint16_t dataMgmtTrimCap;
    uint16_t productID[4];
    uint16_t reserved6[2];
    uint16_t mediaSerial[30];
    uint16_t sctCap;
    uint16_t reserved7[2];
    uint16_t logicalAlignment;
    uint32_t WRVCount3;
    uint32_t WRVCount2;
    uint16_t ob12[3];
    uint16_t rotationRate;
    uint16_t reserved8;
    uint16_t ob13;
    uint16_t WRVMode;
    uint16_t reserved9;
    uint16_t transportMajorRevision;
    uint16_t transportMinorRevision;
    uint16_t reserved10[6];
    uint64_t extendedSectors;
    uint16_t minSectorsMicrocode;
    uint16_t maxSectorsMicrocode;
    uint16_t reserved11[19];
    uint16_t checksum;
}__attribute__((packed)) IdentifyDevice;

/* internal structures used to represent devices */
typedef struct IDEController IDEController;

typedef struct {
    IdentifyDevice identify;
    IDEController *controller;
    uint64_t size;      // number of sectors
    uint16_t sectorSize;
    char serial[21];
    char model[41];
    int lba28, lba48;
} ATADevice;

struct IDEController {
    uint16_t primaryBase;
    uint16_t primaryStatus;
    uint16_t secondaryBase;
    uint16_t secondaryStatus;

    ATADevice primary[2];
    ATADevice secondary[2];

    struct IDEController *next;
};

void ideInit(const char *, uint8_t);
ATADevice *ideGetDrive(uint64_t);
void ataDelay(uint16_t);
int ataIdentify(IDEController *, int, int);

extern IDEController *controllers;
extern int controllerCount;
