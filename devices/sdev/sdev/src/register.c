/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * sdev: Abstraction for storage devices under /dev/sdX
 */

#include <sdev/sdev.h>
#include <liblux/liblux.h>
#include <liblux/sdev.h>
#include <liblux/devfs.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int drvCount = 0;
int devCount = 0;
StorageDevice *sdev = NULL;

/* registerDevice(): registers a storage device
 * params: sd - socket of the driver handling this device
 * params: cmd - register command message
 * returns: nothing
 */

void registerDevice(int sd, SDevRegisterCommand *cmd) {
    // allocate a devfs device
    DevfsRegisterCommand *regcmd = calloc(1, sizeof(DevfsRegisterCommand));
    StorageDevice *dev = calloc(1, sizeof(StorageDevice));
    if(!regcmd || !dev) {
        luxLogf(KPRINT_LEVEL_WARNING, "unable to allocate memory to register storage device\n");
        return;
    }

    regcmd->header.command = COMMAND_DEVFS_REGISTER;
    regcmd->header.length = sizeof(DevfsRegisterCommand);

    // block device owned by root:root and permissions rw-r--r--
    regcmd->status.st_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH | S_IFBLK;
    regcmd->status.st_size = cmd->size * cmd->sectorSize;
    regcmd->status.st_blksize = cmd->sectorSize;
    regcmd->status.st_blocks = cmd->size;

    strcpy(regcmd->server, cmd->server);
    sprintf(regcmd->path, "/sd%d", devCount);
    luxSendDependency(regcmd);

    dev->next = NULL;
    sprintf(dev->name, "/sd%d", devCount);
    strcpy(dev->server, cmd->server);
    dev->deviceID = cmd->device;
    dev->partition = cmd->partitions;
    dev->size = cmd->size;
    dev->sectorSize = cmd->sectorSize;
    dev->root = 1;
    dev->sd = sd;

    devCount++;
    luxLogf(KPRINT_LEVEL_DEBUG, "registered block device /dev%s\n",  dev->name);

    if(!sdev) {
        sdev = dev;
        return;
    }

    StorageDevice *list = sdev;
    while(list->next)
        list = list->next;
    
    list->next = dev;
}

/* findDevice(): finds a storage device by index
 * params: i - index
 * returns: pointer to storage device structure, NULL on error
 */

StorageDevice *findDevice(int i) {
    if(i >= devCount) return NULL;

    StorageDevice *list = sdev;
    int c = 0;
    while(list) {
        if(c == i) return list;
        
        list = list->next;
        c++;
    }

    return NULL;
}