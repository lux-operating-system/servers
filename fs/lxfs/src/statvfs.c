/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * lxfs: Driver for the lxfs file system
 */

#include <errno.h>
#include <liblux/liblux.h>
#include <lxfs/lxfs.h>
#include <sys/statvfs.h>
#include <string.h>

void lxfsStatvfs(StatvfsCommand *cmd) {
    cmd->header.header.response = 1;
    cmd->header.header.length = sizeof(StatvfsCommand);

    Mountpoint *mp = findMP(cmd->device);
    if(!mp) {
        cmd->header.header.status = -EIO;
        luxSendKernel(cmd);
        return;
    }

    memset(&cmd->buffer, 0, sizeof(struct statvfs));
    cmd->buffer.f_bsize = mp->blockSizeBytes;
    cmd->buffer.f_frsize = mp->blockSizeBytes;
    cmd->buffer.f_blocks = mp->volumeSize;
    cmd->buffer.f_flag = ST_NOSUID;
    cmd->buffer.f_namemax = 511;

    cmd->buffer.f_files = cmd->buffer.f_blocks / 2;
    cmd->buffer.f_ffree = cmd->buffer.f_files;

    for(uint64_t i = 0; i < mp->volumeSize; i++) {
        uint64_t block = lxfsNextBlock(mp, i);
        if(!block) cmd->buffer.f_bfree++;
        else if(block == LXFS_BLOCK_EOF) cmd->buffer.f_ffree--;
    }

    cmd->buffer.f_bavail = cmd->buffer.f_bfree;
    cmd->buffer.f_favail = cmd->buffer.f_ffree;
    cmd->buffer.f_fsid = mp->fd;

    cmd->header.header.status = 0;
    luxSendKernel(cmd);
}