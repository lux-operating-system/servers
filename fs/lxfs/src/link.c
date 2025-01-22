/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024-25
 * 
 * lxfs: Driver for the lxfs file system
 */

#include <liblux/liblux.h>
#include <lxfs/lxfs.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

/* lxfsLink(): creates a new hard link on an lxfs volume
 * params: cmd - link command message
 * returns: nothing, response relayed to kernel
 */

void lxfsLink(LinkCommand *cmd) {
    cmd->header.header.response = 1;
    cmd->header.header.length = sizeof(LinkCommand);

    Mountpoint *mp = findMP(cmd->device);
    if(!mp) {
        cmd->header.header.status = -EIO;
        luxSendKernel(cmd);
        return;
    }

    // ensure that the old file exists and that the new file doesn't
    LXFSDirectoryEntry oldFile, newFile;
    if(!lxfsFind(&oldFile, mp, cmd->oldPath, NULL, NULL)) {
        cmd->header.header.status = -ENOENT;
        luxSendKernel(cmd);
        return;
    }

    if(lxfsFind(&newFile, mp, cmd->newPath, NULL, NULL)) {
        cmd->header.header.status = -EEXIST;
        luxSendKernel(cmd);
        return;
    }

    // ensure that the target is a regular file or a hard link
    uint8_t type = (oldFile.flags >> LXFS_DIR_TYPE_SHIFT) & LXFS_DIR_TYPE_MASK;
    if((type != LXFS_DIR_TYPE_FILE) && (type != LXFS_DIR_TYPE_HARD_LINK)) {
        cmd->header.header.status = -EPERM;
        luxSendKernel(cmd);
        return;
    }

    mode_t mode = S_IFREG;
    if(oldFile.permissions & LXFS_PERMS_OWNER_R) mode |= S_IRUSR;
    if(oldFile.permissions & LXFS_PERMS_OWNER_W) mode |= S_IWUSR;
    if(oldFile.permissions & LXFS_PERMS_OWNER_X) mode |= S_IXUSR;
    if(oldFile.permissions & LXFS_PERMS_GROUP_R) mode |= S_IRGRP;
    if(oldFile.permissions & LXFS_PERMS_GROUP_W) mode |= S_IWGRP;
    if(oldFile.permissions & LXFS_PERMS_GROUP_X) mode |= S_IXGRP;
    if(oldFile.permissions & LXFS_PERMS_OTHER_R) mode |= S_IROTH;
    if(oldFile.permissions & LXFS_PERMS_OTHER_W) mode |= S_IWOTH;
    if(oldFile.permissions & LXFS_PERMS_OTHER_X) mode |= S_IXOTH;

    newFile.block = oldFile.block;
    cmd->header.header.status = lxfsCreate(&newFile, mp, cmd->newPath, mode, cmd->uid, cmd->gid);
    luxSendKernel(cmd);
}

/* lxfsUnlink(): removes a link to a file or directory
 * params: cmd - unlink command message
 * returns: nothing, response relayed to kernel
 */

void lxfsUnlink(UnlinkCommand *cmd) {
    cmd->header.header.response = 1;
    cmd->header.header.length = sizeof(UnlinkCommand);

    // special case so we can't delete the root directory
    if(strlen(cmd->path) <= 1) {
        cmd->header.header.status = -EPERM;
        luxSendKernel(cmd);
        return;
    }

    Mountpoint *mp = findMP(cmd->device);
    if(!mp) {
        cmd->header.header.status = -EIO;
        luxSendKernel(cmd);
        return;
    }

    LXFSDirectoryEntry entry;
    uint64_t block;
    off_t offset;
    if(!lxfsFind(&entry, mp, cmd->path, &block, &offset)) {
        cmd->header.header.status = -ENOENT;
        luxSendKernel(cmd);
        return;
    }

    // permission check
    cmd->header.header.status = 0;
    if(cmd->uid == entry.owner) {
        if(!(entry.permissions & LXFS_PERMS_OWNER_W)) cmd->header.header.status = -EPERM;
    } else if(cmd->gid == entry.group) {
        if(!(entry.permissions & LXFS_PERMS_GROUP_W)) cmd->header.header.status = -EPERM;
    } else {
        if(!(entry.permissions & LXFS_PERMS_OTHER_W)) cmd->header.header.status = -EPERM;
    }

    if(cmd->header.header.status) {
        luxSendKernel(cmd);
        return;
    }

    // ensure non-empty directory
    uint8_t type = (entry.flags >> LXFS_DIR_TYPE_SHIFT) & LXFS_DIR_TYPE_MASK;
    if(type == LXFS_DIR_TYPE_DIR) {
        if(lxfsReadBlock(mp, entry.block, mp->meta)) {
            cmd->header.header.status = -EIO;
            luxSendKernel(cmd);
            return;
        }

        LXFSDirectoryHeader *dirHeader = (LXFSDirectoryHeader *) mp->meta;
        if(dirHeader->sizeEntries) {
            cmd->header.header.status = -ENOTEMPTY;
            luxSendKernel(cmd);
            return;
        }
    }

    // delete the associated directory entry
    LXFSDirectoryEntry *dir = (LXFSDirectoryEntry *)((uintptr_t)mp->dataBuffer+offset);
    dir->flags = LXFS_DIR_DELETED;
    dir->block = 0;
    dir->permissions = 0;
    dir->createTime = 0;
    dir->accessTime = 0;
    dir->modTime = 0;
    dir->owner = 0;
    dir->group = 0;
    memset(dir->name, 0, dir->entrySize - offsetof(LXFSDirectoryEntry, name));
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

    // for regular files and hard links, decrement file ref counter
    if((type == LXFS_DIR_TYPE_FILE) || (type == LXFS_DIR_TYPE_HARD_LINK)) {
        if(lxfsReadBlock(mp, entry.block, mp->meta)) {
            cmd->header.header.status = -EIO;
            luxSendKernel(cmd);
            return;
        }

        LXFSFileHeader *fileHeader = (LXFSFileHeader *) mp->meta;
        fileHeader->refCount--;
        if(fileHeader->refCount) {
            // references still exist, update the count
            if(lxfsWriteBlock(mp, entry.block, mp->meta)) {
                cmd->header.header.status = -EIO;
                luxSendKernel(cmd);
                return;
            }

            lxfsFlushBlock(mp, entry.block);
        } else {
            // last reference deleted, free up all blocks used by the file
            uint64_t prev = entry.block;
            while(prev && prev != LXFS_BLOCK_EOF) {
                next = lxfsNextBlock(mp, prev);
                if(lxfsSetNextBlock(mp, prev, 0)) {
                    cmd->header.header.status = -EIO;
                    luxSendKernel(cmd);
                    return;
                }

                prev = next;
            }
        }
    } else {
        // for symbolic links and directories, free up the blocks
        uint64_t prev = entry.block;
        while(prev && prev != LXFS_BLOCK_EOF) {
            next = lxfsNextBlock(mp, prev);
            if(lxfsSetNextBlock(mp, prev, 0)) {
                cmd->header.header.status = -EIO;
                luxSendKernel(cmd);
                return;
            }

            prev = next;
        }
    }

    // update the entry count and timestamps of the parent directory
    LXFSDirectoryEntry parent;
    int depth = pathDepth(cmd->path);
    if(depth <= 1) {
        if(!lxfsFind(&parent, mp, "/", NULL, NULL)) {
            cmd->header.header.status = -EIO;
            luxSendKernel(cmd);
            return;
        }
    } else {
        char *parentPath = strdup(cmd->path);
        if(!parentPath) {
            cmd->header.header.status = -ENOMEM;
            luxSendKernel(cmd);
            return;
        }

        char *last = strrchr(parentPath, '/');
        if(!last) {
            free(parentPath);
            cmd->header.header.status = -ENOENT;
            luxSendKernel(cmd);
            return;
        }

        *last = 0;  // truncate to keep only the parent directory

        if(!lxfsFind(&parent, mp, parentPath, NULL, NULL)) {
            free(parentPath);
            cmd->header.header.status = -ENOENT;
            luxSendKernel(cmd);
            return;
        }

        free(parentPath);
    }

    if(lxfsReadBlock(mp, parent.block, mp->meta)) {
        cmd->header.header.status = -EIO;
        luxSendKernel(cmd);
        return;
    }

    time_t timestamp = time(NULL);
    LXFSDirectoryHeader *parentHeader = (LXFSDirectoryHeader *) mp->meta;
    parentHeader->sizeEntries--;
    parentHeader->accessTime = timestamp;
    parentHeader->modTime = timestamp;

    if(lxfsWriteBlock(mp, parent.block, mp->meta)) {
        cmd->header.header.status = -EIO;
        luxSendKernel(cmd);
        return;
    }

    lxfsFlushBlock(mp, parent.block);
    cmd->header.header.status = 0;
    luxSendKernel(cmd);
}

/* lxfsSymlink(): creates a symbolic link to a file or directory
 * params: cmd - symlink command message
 * returns: nothing, response relayed to kernel
 */

void lxfsSymlink(LinkCommand *cmd) {
    cmd->header.header.response = 1;
    cmd->header.header.length = sizeof(LinkCommand);

    Mountpoint *mp = findMP(cmd->device);
    if(!mp) {
        cmd->header.header.status = -EIO;
        luxSendKernel(cmd);
        return;
    }

    // ensure the new file doesn't already exist
    LXFSDirectoryEntry new;
    if(lxfsFind(&new, mp, cmd->newPath, NULL, NULL)) {
        cmd->header.header.status = -EEXIST;
        luxSendKernel(cmd);
        return;
    }

    // POSIX doesn't specify mode bits for symbolic links so we can innovate
    // here -- if the target file exists, copy the mode from it
    // if not, default to rw-r--r--
    mode_t mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH;
    LXFSDirectoryEntry entry;
    if(lxfsFind(&entry, mp, cmd->oldPath, NULL, NULL)) {
        mode = 0;
        if(entry.permissions & LXFS_PERMS_OWNER_R) mode |= S_IRUSR;
        if(entry.permissions & LXFS_PERMS_OWNER_W) mode |= S_IWUSR;
        if(entry.permissions & LXFS_PERMS_OWNER_X) mode |= S_IXUSR;
        if(entry.permissions & LXFS_PERMS_GROUP_R) mode |= S_IRGRP;
        if(entry.permissions & LXFS_PERMS_GROUP_W) mode |= S_IWGRP;
        if(entry.permissions & LXFS_PERMS_GROUP_X) mode |= S_IXGRP;
        if(entry.permissions & LXFS_PERMS_OTHER_R) mode |= S_IROTH;
        if(entry.permissions & LXFS_PERMS_OTHER_W) mode |= S_IWOTH;
        if(entry.permissions & LXFS_PERMS_OTHER_X) mode |= S_IXOTH;
    }

    mode |= S_IFLNK;
    entry.block = 0;
    cmd->header.header.status = lxfsCreate(&entry, mp, cmd->newPath, mode, cmd->uid, cmd->gid, cmd->oldPath);
    luxSendKernel(cmd);
}

/* lxfsReadLink(): reads the contents of a symbolic link
 * params: cmd - readlink command message
 * returns: nothing, response relayed to kernel
 */

void lxfsReadLink(ReadLinkCommand *cmd) {
    cmd->header.header.response = 1;
    cmd->header.header.length = sizeof(ReadLinkCommand);

    Mountpoint *mp = findMP(cmd->device);
    if(!mp) {
        cmd->header.header.status = -EIO;
        luxSendKernel(cmd);
        return;
    }

    LXFSDirectoryEntry entry;
    if(!lxfsFind(&entry, mp, cmd->path, NULL, NULL)) {
        cmd->header.header.status = -ENOENT;
        luxSendKernel(cmd);
        return;
    }

    uint8_t type = (entry.flags >> LXFS_DIR_TYPE_SHIFT) & LXFS_DIR_TYPE_MASK;
    if(type != LXFS_DIR_TYPE_SOFT_LINK) {
        cmd->header.header.status = -EINVAL;
        luxSendKernel(cmd);
        return;
    }

    if(lxfsReadBlock(mp, entry.block, mp->meta)) {
        cmd->header.header.status = -EIO;
        luxSendKernel(cmd);
        return;
    }

    memset(cmd->path, 0, sizeof(cmd->path));
    size_t truelen = entry.size;
    if(truelen > sizeof(cmd->path)) truelen = sizeof(cmd->path);
    memcpy(cmd->path, mp->meta, truelen);
    
    cmd->header.header.status = truelen;
    luxSendKernel(cmd);
}