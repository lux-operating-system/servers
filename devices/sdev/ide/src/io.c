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
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* ataDelay(): delays the I/O bus by reading from the status port
 * params: base - base I/O port of an IDE controller
 * returns: nothing
 */

void ataDelay(uint16_t base) {
    for(int i = 0; i < 4; i++)
        inb(base + ATA_COMMAND_STATUS);
}

/* ideRead(): handler for read requests for an IDE ATA drive
 * params: cmd - read command message
 * returns: nothing, response relayed to sdev server
 */

void ideRead(SDevRWCommand *cmd) {
    cmd->header.response = 1;

    ATADevice *dev = ideGetDrive(cmd->device);
    if(!dev) {
        cmd->header.status = -ENODEV;
        luxSendDependency(cmd);
        return;
    }

    if((cmd->start % dev->sectorSize) || (cmd->count % dev->sectorSize)) {
        cmd->header.status = -EIO;
        luxSendDependency(cmd);
        return;
    }

    SDevRWCommand *res = malloc(sizeof(SDevRWCommand) + cmd->count);
    if(!res) {
        cmd->header.status = -ENOMEM;
        luxSendDependency(cmd);
        return;
    }

    memcpy(res, cmd, sizeof(SDevRWCommand));

    uint64_t lba = cmd->start / dev->sectorSize;
    uint16_t count = cmd->count / dev->sectorSize;

    if(ataReadSector(dev, lba, count, res->buffer)) {
        luxLogf(KPRINT_LEVEL_WARNING, "I/O error on %s channel port %d\n",
            dev->channel ? "secondary" : "primary", dev->port);
        free(res);
        cmd->header.status = -EIO;
        luxSendDependency(cmd);
        return;
    }

    res->header.status = 0;
    res->header.length = sizeof(SDevRWCommand) + cmd->count;
    luxSendDependency(res);
    free(res);
}

/* ideWrite(): handler for write requests for an IDE ATA drive
 * params: cmd - write command message
 * returns: nothing, response relayed to sdev server
 */

void ideWrite(SDevRWCommand *cmd) {
    cmd->header.response = 1;
    cmd->header.length = sizeof(SDevRWCommand);

    ATADevice *dev = ideGetDrive(cmd->device);
    if(!dev) {
        cmd->header.status = -ENODEV;
        luxSendDependency(cmd);
        return;
    }

    if((cmd->start % dev->sectorSize) || (cmd->count % dev->sectorSize)) {
        cmd->header.status = -EIO;
        luxSendDependency(cmd);
        return;
    }

    uint64_t lba = cmd->start / dev->sectorSize;
    uint16_t count = cmd->count / dev->sectorSize;

    if(ataWriteSector(dev, lba, count, cmd->buffer)) {
        luxLogf(KPRINT_LEVEL_WARNING, "I/O error on %s channel port %d\n",
            dev->channel ? "secondary" : "primary", dev->port);
        cmd->header.status = -EIO;
        luxSendDependency(cmd);
        return;
    }

    cmd->header.status = 0;
    luxSendDependency(cmd);
}
