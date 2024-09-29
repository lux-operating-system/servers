/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * lxfs: Driver for the lxfs file system
 */

#include <liblux/liblux.h>
#include <lxfs/lxfs.h>
#include <vfs.h>
#include <string.h>
#include <stdlib.h>
#include <fnctl.h>
#include <unistd.h>
#include <errno.h>

static Mountpoint *mps = NULL;

static Mountpoint *allocateMP() {
    Mountpoint *mp = calloc(1, sizeof(Mountpoint));
    if(!mp) return NULL;

    if(!mps) {
        mps = mp;
        return mp;
    }

    Mountpoint *list = mps;
    while(list->next) list = list->next;
    list->next = mp;

    return mp;
}

void lxfsMount(MountCommand *cmd) {
    cmd->header.header.response = 1;

    // read the LXFS identification block and ensure this is a valid partition
    LXFSIdentification *id = malloc(4096);
    if(!id) {
        cmd->header.header.status = -ENOMEM;
        luxSendDependency(cmd);
        return;
    }

    int fd = open(cmd->source, O_RDWR);
    if(fd < 0) {
        // relay errors back to the dependency
        cmd->header.header.status = -1*errno;
        luxSendDependency(cmd);
        return;
    }

    if(read(fd, id, 512) < sizeof(LXFSIdentification)) {
        cmd->header.header.status = -1*errno;
        close(fd);
        free(id);
        luxSendDependency(cmd);
        return;
    }

    if(id->identifier != LXFS_MAGIC) {
        cmd->header.header.status = -ENODEV;
        close(fd);
        free(id);
        luxSendDependency(cmd);
        return;
    }

    int sectorSize = 512 << ((id->parameters >> 1) & 3);
    int blockSize = ((id->parameters >> 3) & 0x0F) + 1;
    int blockSizeBytes = sectorSize * blockSize;

    void *buffer = malloc(blockSizeBytes);
    if(!buffer) {
        cmd->header.header.status = -ENOMEM;
        close(fd);
        free(id);
        luxSendDependency(cmd);
        return;
    }

    void *buffer2 = malloc(blockSizeBytes);
    if(!buffer2) {
        cmd->header.header.status = -ENOMEM;
        close(fd);
        free(id);
        free(buffer);
        luxSendDependency(cmd);
        return;
    }

    Mountpoint *mp = allocateMP();
    if(!mp) {
        cmd->header.header.status = -ENOMEM;
        close(fd);
        free(id);
        free(buffer);
        free(buffer2);
        luxSendDependency(cmd);
        return;
    }

    luxLogf(KPRINT_LEVEL_DEBUG, "mounted lxfs volume on %s:\n", cmd->source);

    strcpy(mp->device, cmd->source);
    mp->fd = fd;
    mp->sectorSize = sectorSize;
    mp->blockSize = blockSize;
    mp->blockSizeBytes = blockSizeBytes;
    mp->root = id->rootBlock;
    mp->blockTableBuffer = buffer;
    mp->dataBuffer = buffer2;

    luxLogf(KPRINT_LEVEL_DEBUG, "- %d bytes per sector, %d sectors per block\n", mp->sectorSize, mp->blockSize);
    luxLogf(KPRINT_LEVEL_DEBUG, "- root directory at block %d\n", mp->root);

    cmd->header.header.status = 0;
    luxSendDependency(cmd);
}