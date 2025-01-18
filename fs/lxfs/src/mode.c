/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024-25
 * 
 * lxfs: Driver for the lxfs file system
 */

#include <lxfs/lxfs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

/* lxfsChmod(): implementation of chmod() for lxfs
 * params: cmd - chmod command message
 * returns: nothing, response relayed to kernel
 */

void lxfsChmod(ChmodCommand *cmd) {
    cmd->header.header.response = 1;
    cmd->header.header.length = sizeof(ChmodCommand);

    Mountpoint *mp = findMP(cmd->device);
    if(!mp) {
        cmd->header.header.status = -EIO;
        luxSendKernel(cmd);
        return;
    }

    uint64_t block;
    off_t offset;
    LXFSDirectoryEntry entry;
    if(!lxfsFind(&entry, mp, cmd->path, &block, &offset)) {
        cmd->header.header.status = -ENOENT;
        luxSendKernel(cmd);
        return;
    }

    // only the owner of the file can edit permissions
    if(entry.owner != cmd->uid) {
        cmd->header.header.status = -EPERM;
        luxSendKernel(cmd);
        return;
    }

    LXFSDirectoryEntry *dir = (LXFSDirectoryEntry *)((uintptr_t)mp->dataBuffer + offset);

    dir->permissions = 0;
    if(cmd->mode & S_IRUSR) dir->permissions |= LXFS_PERMS_OWNER_R;
    if(cmd->mode & S_IWUSR) dir->permissions |= LXFS_PERMS_OWNER_W;
    if(cmd->mode & S_IXUSR) dir->permissions |= LXFS_PERMS_OWNER_X;
    if(cmd->mode & S_IRGRP) dir->permissions |= LXFS_PERMS_GROUP_R;
    if(cmd->mode & S_IWGRP) dir->permissions |= LXFS_PERMS_GROUP_W;
    if(cmd->mode & S_IXGRP) dir->permissions |= LXFS_PERMS_GROUP_X;
    if(cmd->mode & S_IROTH) dir->permissions |= LXFS_PERMS_OTHER_R;
    if(cmd->mode & S_IWOTH) dir->permissions |= LXFS_PERMS_OTHER_W;
    if(cmd->mode & S_IXOTH) dir->permissions |= LXFS_PERMS_OTHER_X;

    uint64_t next = lxfsWriteNextBlock(mp, block, mp->dataBuffer);
    if(!next) {
        cmd->header.header.status = -EIO;
        luxSendKernel(cmd);
        return;
    }

    lxfsFlushBlock(mp, block);

    if((offset + entry.entrySize) > mp->blockSizeBytes) {
        if(lxfsWriteBlock(mp, next, (const void *)((uintptr_t)mp->dataBuffer + mp->blockSizeBytes))) {
            cmd->header.header.status = -EIO;
            luxSendKernel(cmd);
            return;
        }

        lxfsFlushBlock(mp, next);
    }

    cmd->header.header.status = 0;
    luxSendKernel(cmd);
    return;
}

/* lxfsChown(): implementation of chown() for lxfs
 * params: cmd - chown command message
 * returns: nothing, response relayed to kernel
 */

void lxfsChown(ChownCommand *cmd) {
    cmd->header.header.response = 1;
    cmd->header.header.length = sizeof(ChownCommand);

    // short circuit conditional for when uid == gid == -1
    if((cmd->newUid == -1) && (cmd->newGid == -1)) {
        cmd->header.header.status = 0;
        luxSendKernel(cmd);
        return;
    }

    Mountpoint *mp = findMP(cmd->device);
    if(!mp) {
        cmd->header.header.status = -EIO;
        luxSendKernel(cmd);
        return;
    }

    uint64_t block;
    off_t offset;
    LXFSDirectoryEntry entry;
    if(!lxfsFind(&entry, mp, cmd->path, &block, &offset)) {
        cmd->header.header.status = -ENOENT;
        luxSendKernel(cmd);
        return;
    }

    // only the owner of the file can change the owner
    if(entry.owner != cmd->uid) {
        cmd->header.header.status = -EPERM;
        luxSendKernel(cmd);
        return;
    }

    LXFSDirectoryEntry *dir = (LXFSDirectoryEntry *)((uintptr_t)mp->dataBuffer + offset);
    if(cmd->newUid != -1) dir->owner = cmd->newUid;
    if(cmd->newGid != -1) dir->group = cmd->newGid;

    uint64_t next = lxfsWriteNextBlock(mp, block, mp->dataBuffer);
    if(!next) {
        cmd->header.header.status = -EIO;
        luxSendKernel(cmd);
        return;
    }

    lxfsFlushBlock(mp, block);

    if((offset + entry.entrySize) > mp->blockSizeBytes) {
        if(lxfsWriteBlock(mp, next, (const void *)((uintptr_t)mp->dataBuffer + mp->blockSizeBytes))) {
            cmd->header.header.status = -EIO;
            luxSendKernel(cmd);
            return;
        }

        lxfsFlushBlock(mp, next);
    }

    cmd->header.header.status = 0;
    luxSendKernel(cmd);
    return;
}

/* lxfsUtime(): implementation of utime() for lxfs
 * params: cmd - utime command message
 * returns: nothing, response relayed to kernel
 */

void lxfsUtime(UtimeCommand *cmd) {
    cmd->header.header.response = 1;
    cmd->header.header.length = sizeof(UtimeCommand);

    Mountpoint *mp = findMP(cmd->device);
    if(!mp) {
        cmd->header.header.status = -EIO;
        luxSendKernel(cmd);
        return;
    }

    uint64_t block;
    off_t offset;
    LXFSDirectoryEntry entry;
    if(!lxfsFind(&entry, mp, cmd->path, &block, &offset)) {
        cmd->header.header.status = -ENOENT;
        luxSendKernel(cmd);
        return;
    }

    // only file owner or someone else with write perms can do this
    cmd->header.header.status = 0;
    if(cmd->uid != entry.owner) {
        if((cmd->gid == entry.group) && !(entry.permissions & LXFS_PERMS_GROUP_W))
            cmd->header.header.status = -EPERM;
        else if(!(entry.permissions & LXFS_PERMS_OTHER_W))
            cmd->header.header.status = -EPERM;
    }

    if(cmd->header.header.status) {
        luxSendKernel(cmd);
        return;
    }

    LXFSDirectoryEntry *dir = (LXFSDirectoryEntry *)((uintptr_t)mp->dataBuffer + offset);
    dir->accessTime = cmd->accessTime;
    dir->modTime = cmd->modifiedTime;

    uint64_t next = lxfsWriteNextBlock(mp, block, mp->dataBuffer);
    if(!next) {
        cmd->header.header.status = -EIO;
        luxSendKernel(cmd);
        return;
    }

    lxfsFlushBlock(mp, block);

    if((offset + entry.entrySize) > mp->blockSizeBytes) {
        if(lxfsWriteBlock(mp, next, (const void *)((uintptr_t)mp->dataBuffer + mp->blockSizeBytes))) {
            cmd->header.header.status = -EIO;
            luxSendKernel(cmd);
            return;
        }

        lxfsFlushBlock(mp, next);
    }

    // update the directory header as well
    if(((dir->flags >> LXFS_DIR_TYPE_SHIFT) & LXFS_DIR_TYPE_MASK) == LXFS_DIR_TYPE_DIR) {
        LXFSDirectoryHeader *dirHeader = (LXFSDirectoryHeader *) mp->dataBuffer;
        if(lxfsReadBlock(mp, entry.block, mp->dataBuffer)) {
            cmd->header.header.status = -EIO;
            luxSendKernel(cmd);
            return;
        }

        dirHeader->accessTime = cmd->accessTime;
        dirHeader->modTime = cmd->modifiedTime;
        if(lxfsWriteBlock(mp, entry.block, mp->dataBuffer)) {
            cmd->header.header.status = -EIO;
            luxSendKernel(cmd);
            return;
        }

        lxfsFlushBlock(mp, entry.block);
    }

    cmd->header.header.status = 0;
    luxSendKernel(cmd);
    return;
}