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
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

/* lxfsMmap(): implementation of mmap() for lxfs
 * params: cmd - mmap command message
 * returns: nothing, response relayed to kernel
 */

void lxfsMmap(MmapCommand *cmd) {
    cmd->header.header.response = 1;
    cmd->header.header.length = sizeof(MmapCommand);

    // get the mountpoint
    Mountpoint *mp = findMP(cmd->device);
    if(!mp) {
        cmd->header.header.status = -EIO;
        luxSendKernel(cmd);
        return;
    }

    // and the file entry
    LXFSDirectoryEntry entry;
    if(!lxfsFind(&entry, mp, cmd->path)) {
        cmd->header.header.status = -ENOENT;
        luxSendKernel(cmd);
        return;
    }

    // use the file entry to read metadata as well as find the first file block
    uint64_t first = lxfsReadNextBlock(mp, entry.block, mp->meta);
    if(!first) {
        cmd->header.header.status = -EIO;
        luxSendKernel(cmd);
        return;
    }

    LXFSFileHeader *metadata = (LXFSFileHeader *) mp->meta;

    if(cmd->len > metadata->size)
        cmd->len = metadata->size;

    size_t blockCount = (cmd->len+mp->blockSizeBytes-1) / mp->blockSizeBytes;

    MmapCommand *res = calloc(1, sizeof(MmapCommand) + cmd->len);
    if(!res) {
        cmd->header.header.status = -ENOMEM;
        luxSendKernel(cmd);
        return;
    }

    memcpy(res, cmd, sizeof(MmapCommand));
    res->responseType = 0;
    res->mmio = 0;

    uint64_t block = first;
    void *position = (void *) res->data;
    for(size_t i = 0; i < blockCount; i++) {
        block = lxfsReadNextBlock(mp, block, mp->dataBuffer);
        if(!block) {
            res->header.header.status = -EIO;
            luxSendKernel(res);
            free(res);
            return;
        }

        memcpy(position, mp->dataBuffer, mp->blockSizeBytes);

        if(block == LXFS_BLOCK_EOF) break;
        position = (void *)(uintptr_t) position + mp->blockSizeBytes;
    }

    res->header.header.length += cmd->len;
    luxSendKernel(res);
    free(res);
}