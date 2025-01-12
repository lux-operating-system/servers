/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * lxfs: Driver for the lxfs file system
 */

#include <liblux/liblux.h>
#include <lxfs/lxfs.h>
#include <errno.h>

/* lxfsFsync(): implementation of fsync() for lxfs
 * params: cmd - fsync command message
 * returns: nothing, response relayed to kernel
 */

void lxfsFsync(FsyncCommand *cmd) {
    cmd->header.header.response = 1;
    cmd->header.header.length = sizeof(FsyncCommand);

    Mountpoint *mp = findMP(cmd->device);
    if(!mp) {
        cmd->header.header.status = -EIO;
        luxSendKernel(cmd);
        return;
    }

    LXFSDirectoryEntry entry;
    if(!lxfsFind(&entry, mp, cmd->path, NULL, NULL)) {
        if(!cmd->close) cmd->header.header.status = -ENOENT;
        else cmd->header.header.status = 0;
        luxSendKernel(cmd);
        return;
    }

    uint64_t next = entry.block;
    while(next && (next != LXFS_BLOCK_EOF)) {
        if(lxfsFlushBlock(mp, next)) {
            cmd->header.header.status = -EIO;
            luxSendKernel(cmd);
            return;
        }

        next = lxfsNextBlock(mp, next);
        if(!next) {
            cmd->header.header.status = -EIO;
            luxSendKernel(cmd);
            return;
        }
    }

    cmd->header.header.status = 0;
    luxSendKernel(cmd);
}