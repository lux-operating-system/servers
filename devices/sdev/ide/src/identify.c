/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024-25
 * 
 * ide: Device driver for IDE (ATA HDDs)
 */

#include <liblux/liblux.h>
#include <liblux/sdev.h>
#include <ide/ide.h>
#include <sys/io.h>
#include <unistd.h>
#include <string.h>

#define IDENTIFY_TIMEOUT            20  /* in units of scheduler yields */

/* ataIdentify(): identifies an ATA device
 * params: ctrl - IDE controller to which the drive is attached
 * params: channel - 0 for primary channel, 1 for secondary
 * params: drive - 0 or 1 for device on channel
 * returns: zero on success
 */

int ataIdentify(IDEController *ctrl, int channel, int drive) {
    channel &= 1;
    drive &= 1;
    uint16_t port;
    if(channel) port = ctrl->secondaryBase;
    else port = ctrl->primaryBase;

    ATADevice *dev;
    if(!channel) dev = &ctrl->primary[drive&1];
    else dev = &ctrl->secondary[drive&1];

    dev->valid = 0;

    outb(port + ATA_DRIVE_SELECT, 0xA0 | (drive << 4));
    ataDelay(port);
    outb(port + ATA_SECTOR_COUNT, 0);
    outb(port + ATA_LBA_LOW, 0);
    outb(port + ATA_LBA_MID, 0);
    outb(port + ATA_LBA_HIGH, 0);
    outb(port + ATA_COMMAND_STATUS, ATA_IDENTIFY);
    ataDelay(port);

    uint8_t status = inb(port + ATA_COMMAND_STATUS);
    if(!status || (status == 0xFF)) {
        luxLogf(KPRINT_LEVEL_DEBUG, " - %s port %d: not present\n",
            channel ? "secondary" : "primary", drive);
        return -1;
    }

    if(inb(port + ATA_LBA_HIGH) == 0xEB) {
        luxLogf(KPRINT_LEVEL_WARNING, " - %s port %d: unimplemented ATAPI device\n",
            channel ? "secondary" : "primary", drive);
        return -1;
    }

    int timeout = 0;
    while(inb(port + ATA_COMMAND_STATUS) & ATA_STATUS_BUSY) {
        timeout++;
        if(timeout >= IDENTIFY_TIMEOUT) {
            luxLogf(KPRINT_LEVEL_WARNING, " - %s port %d: operation timed out\n",
                channel ? "secondary" : "primary", drive);
            return -1;
        }

        sched_yield();
    }

    timeout = 0;
    while(!(status = inb(port + ATA_COMMAND_STATUS) & ATA_STATUS_DATA_REQUEST)) {
        if((status & ATA_STATUS_DRIVE_FAULT) || (status & ATA_STATUS_ERROR)) {
            luxLogf(KPRINT_LEVEL_WARNING, " - %s port %d: %s\n",
                channel ? "secondary" : "primary", drive,
                status & ATA_STATUS_DRIVE_FAULT ? "drive fault" : "general I/O error");
        }
        timeout++;
        if(timeout >= IDENTIFY_TIMEOUT) {
            luxLogf(KPRINT_LEVEL_WARNING, " - %s port %d: operation timed out\n",
                channel ? "secondary" : "primary", drive);
            return -1;
        }

        sched_yield();
    }

    uint16_t *raw = (uint16_t *) &dev->identify;
    for(int i = 0; i < 256; i++) {
        raw[i] = inw(port);
    }

    memset(dev->model, 0, sizeof(dev->model));
    memset(dev->serial, 0, sizeof(dev->serial));
    strncpy(dev->model, (const char *) dev->identify.model, sizeof(dev->model)-1);
    strncpy(dev->serial, (const char *) dev->identify.serial, sizeof(dev->serial)-1);

    // correct endianness of strings
    for(int i = 0; i < sizeof(dev->model) / 2; i++) {
        char temp = dev->model[i*2];
        dev->model[i*2] = dev->model[(i*2)+1];
        dev->model[(i*2)+1] = temp;
    }

    for(int i = 0; i < sizeof(dev->model)-1; i++) {
        if(dev->model[i] == ' ' && dev->model[i+1] == ' ') {
            dev->model[i] = 0;
            break;
        }
    }

    for(int i = 0; i < sizeof(dev->serial) / 2; i++) {
        char temp = dev->serial[i*2];
        dev->serial[(i*2)] = dev->serial[(i*2)+1];
        dev->serial[(i*2)+1] = temp;
    }

    for(int i = 0; i < sizeof(dev->serial)-1; i++) {
        if(dev->serial[i] == ' ' && dev->serial[i+1] == ' ') {
            dev->serial[i] = 0;
            break;
        }
    }

    if(!(dev->identify.cap3 & ATA_CAP3_LBA28)) dev->lba28 = 1;
    else dev->lba28 = 0;

    if((dev->identify.cmdCap2 & ATA_CMDCAP2_LBA48) || (dev->identify.cmdCap5 & ATA_CMDCAP5_LBA48))
        dev->lba48 = 1;
    else dev->lba48 = 0;

    if((!dev->lba28) && (!dev->lba48)) {
        luxLogf(KPRINT_LEVEL_ERROR, " - %s port %d: %s, does not implement LBA, ignoring device\n",
            channel ? "secondary" : "primary", drive, dev->model);
        return -1;
    }

    dev->sectorSize = dev->identify.logicalSectorSize * 2;
    if(!dev->sectorSize) dev->sectorSize = 512;

    if(dev->lba48) dev->size = dev->identify.logicalSize48;
    else dev->size = dev->identify.logicalSize28;

    if(!dev->size) {
        luxLogf(KPRINT_LEVEL_ERROR, " - %s port %d: %s, returned logical size zero, ignoring device\n",
            channel ? "secondary" : "primary", drive, dev->model);
        return -1;
    }

    int readableSize;
    char *unit;
    uint64_t size = dev->size * dev->sectorSize;
    if(size >= 0x10000000000) {
        readableSize = size / 0x10000000000;
        unit = "TiB";
    } else if(size >= 0x40000000) {
        readableSize = size / 0x40000000;
        unit = "GiB";
    } else if(size >= 0x100000) {
        readableSize = size / 0x100000;
        unit = "MiB";
    } else {
        readableSize = size / 1024;
        unit = "KiB";
    }

    luxLogf(KPRINT_LEVEL_DEBUG, " - %s port %d: %s, sector size %d, drive size %d %s, %s%s\n",
        channel ? "secondary" : "primary", drive,
        dev->model, dev->sectorSize,
        readableSize, unit,
        dev->lba28 ? "LBA28 " : "",
        dev->lba48 ? "LBA48 " : "");

    dev->valid = 1;
    dev->controller = ctrl;
    dev->channel = channel;
    dev->port = drive;
    return 0;
}