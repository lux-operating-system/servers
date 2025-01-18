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

/* ataSelect(): helper function to select a drive and send it an address
 * params: port - I/O port base for the IDE channel
 * params: using48 - 48-bit/28-bit addressing mode toggle
 * params: drive - port 0/1 toggle
 * params: lba - starting LBA address
 * params: count - number of sectors
 * returns: nothing */

static void ataSelect(uint16_t port, int using48, int drive, uint64_t lba, uint16_t count) {
    uint8_t selector = (drive & 1) << 4;
    if(!using48)
        selector |= 0xE0 | ((lba >> 24) & 0x0F);  // highest 4 bits of LBA
    else
        selector |= 0x40;

    outb(port + ATA_DRIVE_SELECT, selector);
    ataDelay(port);

    if(using48) {
        // high bits for 48-bit addressing mode
        outb(port + ATA_SECTOR_COUNT, count >> 8);
        outb(port + ATA_LBA_LOW, lba >> 24);
        outb(port + ATA_LBA_MID, lba >> 32);
        outb(port + ATA_LBA_HIGH, lba >> 40);
        ataDelay(port);
    }

    // low bits are common in both 28-bit and 48-bit modes
    outb(port + ATA_SECTOR_COUNT, count);
    outb(port + ATA_LBA_LOW, lba);
    outb(port + ATA_LBA_MID, lba >> 8);
    outb(port + ATA_LBA_HIGH, lba >> 16);
}

/* ataReadSector(): reads contiguous sectors from an ATA drive
 * params: drive - drive to read from
 * params: lba - starting LBA address
 * params: count - number of sectors to read
 * params: buffer - buffer to read into
 * returns: zero on success
 */

int ataReadSector(ATADevice *drive, uint64_t lba, uint16_t count, void *buffer) {
    if(!count) return -1;

    uint16_t port;
    if(drive->channel) port = drive->controller->secondaryBase;
    else port = drive->controller->primaryBase;

    if(!port) return -1;
    if((lba+count) >= drive->size) return -1;

    // prefer 28-bit mode over 48 because less I/O overhead
    int using48 = (lba >= (1 << 28)) || (!drive->lba28);
    if(using48 && (!drive->lba48)) {
        luxLogf(KPRINT_LEVEL_ERROR, "%s channel port %d: tried to read large address from device that doesn't support LBA48\n",
            drive->channel ? "secondary" : "primary", drive->port);
        return -1;
    }

    ataSelect(port, using48, drive->port, lba, count);

    if(using48) outb(port + ATA_COMMAND_STATUS, ATA_READ48);
    else outb(port + ATA_COMMAND_STATUS, ATA_READ28);

    ataDelay(port);
    uint8_t status = inb(port + ATA_COMMAND_STATUS);
    if(!status || (status == 0xFF)) return -1;

    uint16_t *raw = (uint16_t *) buffer;

    for(uint16_t cc = 0; cc < count; cc++) {
        while((status = inb(port + ATA_COMMAND_STATUS)) & ATA_STATUS_BUSY)
            sched_yield();

        while(!(status & ATA_STATUS_DATA_REQUEST)) {
            status = inb(port + ATA_COMMAND_STATUS);
            if((status & ATA_STATUS_ERROR) || (status & ATA_STATUS_DRIVE_FAULT))
                return -1;
            sched_yield();
        }

        for(int i = 0; i < drive->sectorSize/2; i++)
            raw[i] = inw(port);

        raw = (uint16_t *)((uintptr_t) raw + drive->sectorSize);
        ataDelay(port);
    }

    return 0;
}