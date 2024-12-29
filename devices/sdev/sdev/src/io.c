/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * sdev: Abstraction for storage devices under /dev/sdX
 */

#include <liblux/liblux.h>
#include <liblux/sdev.h>
#include <sdev/sdev.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* sdevRead(): reads from a storage device
 * params: cmd - read command message
 * returns: nothing, request relayed to storage device driver
 */

void sdevRead(RWCommand *cmd) {
    int i = atoi(&cmd->path[3]);
    StorageDevice *dev = findDevice(i);
    if(!dev) {
        // no such entry somehow - possible hotpluggable device removed
        cmd->header.header.response = 1;
        cmd->header.header.length = sizeof(RWCommand);
        cmd->header.header.status = -ENODEV;
        cmd->length = 0;
        luxSendDependency(cmd);
        return;
    }

    // check if we're reading from the block device itself or from a partition
    int partition = -1;
    for(int i = 0; i < strlen(cmd->path); i++) {
        if(cmd->path[i] == 'p') partition = atoi(&cmd->path[i+1]);
    }

    // relay the request to the appropriate device driver
    SDevRWCommand rcmd;
    memset(&rcmd, 0, sizeof(SDevRWCommand));
    rcmd.header.command = COMMAND_SDEV_READ;
    rcmd.header.length = sizeof(SDevRWCommand);
    rcmd.syscall = cmd->header.id;
    rcmd.start = cmd->position;
    rcmd.count = cmd->length;
    rcmd.device = dev->deviceID;
    rcmd.pid = cmd->header.header.requester;
    rcmd.partition = partition;
    rcmd.sectorSize = dev->sectorSize;
    
    if(partition != -1) {
        rcmd.partitionStart = dev->partitionStart[partition];
        rcmd.start += dev->partitionStart[partition] * dev->sectorSize;

        // ensure we don't cross partition boundaries
        uint64_t end = dev->partitionStart[partition] + dev->partitionSize[partition];
        uint64_t ioEnd = (rcmd.start + rcmd.count) / dev->sectorSize;
        if(ioEnd > end) {
            // return an I/O error if trying to cross partition boundaries
            cmd->header.header.response = 1;
            cmd->header.header.length = sizeof(RWCommand);
            cmd->header.header.status = -EIO;
            cmd->length = 0;
            luxSendDependency(cmd);
            return;
        }
    }

    luxSend(dev->sd, &rcmd);
}

/* relayRead(): relays the read response from a device driver to the requester
 * params: res - read response message
 * returns: nothing, response relayed to the requester
 */

void relayRead(SDevRWCommand *res) {
    // allocate a buffer of differing size according to the command's status
    if(!res->header.status) {
        // success
        RWCommand *rcmd = calloc(1, sizeof(RWCommand) + res->count);
        if(!rcmd) {
            luxLogf(KPRINT_LEVEL_ERROR, "unable to allocate memory for I/O operations\n");
            return;
        }

        rcmd->header.header.command = COMMAND_READ;
        rcmd->header.header.length = sizeof(RWCommand) + res->count;
        rcmd->header.header.response = 1;
        rcmd->header.header.status = res->count;
        rcmd->header.header.requester = res->pid;
        rcmd->header.id = res->syscall;
        rcmd->position = res->start + res->count;
        rcmd->length = res->count;

        if(res->partition >= 0 && res->partition < 4) {
            rcmd->position -= res->partitionStart * res->sectorSize;
        }
        
        memcpy(rcmd->data, res->buffer, res->count);

        luxSendKernel(rcmd);
        free(rcmd);
    } else {
        // I/O error, simply pass on the error code
        RWCommand rcmd;
        memset(&rcmd, 0, sizeof(RWCommand));

        rcmd.header.header.command = COMMAND_READ;
        rcmd.header.header.length = sizeof(RWCommand);
        rcmd.header.header.response = 1;
        rcmd.header.header.status = res->header.status;
        rcmd.header.header.requester = res->pid;
        rcmd.header.id = res->syscall;
        rcmd.position = res->start;
        rcmd.length = 0;

        luxSendKernel(&rcmd);
    }
}

/* sdevWrite(): writes to a storage device
 * params: cmd - write command message
 * returns: nothing, request relayed to storage device driver
 */

void sdevWrite(RWCommand *cmd) {
    int i = atoi(&cmd->path[3]);
    StorageDevice *dev = findDevice(i);
    if(!dev) {
        // no such entry somehow - possible hotpluggable device removed
        cmd->header.header.response = 1;
        cmd->header.header.length = sizeof(RWCommand);
        cmd->header.header.status = -ENODEV;
        cmd->length = 0;
        luxSendDependency(cmd);
        return;
    }

    // check if we're reading from the block device itself or from a partition
    int partition = -1;
    for(int i = 0; i < strlen(cmd->path); i++) {
        if(cmd->path[i] == 'p') partition = atoi(&cmd->path[i+1]);
    }

    // relay the request to the appropriate device driver
    SDevRWCommand *wcmd = malloc(sizeof(SDevRWCommand) + cmd->length);
    if(!wcmd) {
        cmd->header.header.response = 1;
        cmd->header.header.length = sizeof(RWCommand);
        cmd->header.header.status = -ENOMEM;
        cmd->length = 0;
        luxSendDependency(cmd);
        return;
    }

    memset(wcmd, 0, sizeof(SDevRWCommand));
    wcmd->header.command = COMMAND_SDEV_READ;
    wcmd->header.length = sizeof(SDevRWCommand) + cmd->length;
    wcmd->syscall = cmd->header.id;
    wcmd->start = cmd->position;
    wcmd->count = cmd->length;
    wcmd->device = dev->deviceID;
    wcmd->pid = cmd->header.header.requester;
    wcmd->partition = partition;
    wcmd->sectorSize = dev->sectorSize;
    
    if(partition != -1) {
        wcmd->partitionStart = dev->partitionStart[partition];
        wcmd->start += dev->partitionStart[partition] * dev->sectorSize;

        // ensure we don't cross partition boundaries
        uint64_t end = dev->partitionStart[partition] + dev->partitionSize[partition];
        uint64_t ioEnd = (wcmd->start + wcmd->count) / dev->sectorSize;
        if(ioEnd > end) {
            // return an I/O error if trying to cross partition boundaries
            cmd->header.header.response = 1;
            cmd->header.header.length = sizeof(RWCommand);
            cmd->header.header.status = -EIO;
            cmd->length = 0;
            luxSendDependency(cmd);
            return;
        }
    }

    memcpy(wcmd->buffer, cmd->data, cmd->length);
    luxSend(dev->sd, wcmd);
    free(wcmd);
}