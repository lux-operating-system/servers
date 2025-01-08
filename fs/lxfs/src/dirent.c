/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * lxfs: Driver for the lxfs file system
 */

#include <liblux/liblux.h>
#include <lxfs/lxfs.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

/* lxfsOpendir(): opens a directory on an lxfs volume
 * params: ocmd - open directory command message
 * returns: nothing, response relayed to vfs
 */

void lxfsOpendir(OpendirCommand *ocmd) {
    ocmd->header.header.response = 1;
    ocmd->header.header.length = sizeof(OpendirCommand);

    Mountpoint *mp = findMP(ocmd->device);
    if(!mp) {
        ocmd->header.header.status = -EIO;  // device doesn't exist
        luxSendKernel(ocmd);
        return;
    }

    LXFSDirectoryEntry entry;
    if(!lxfsFind(&entry, mp, ocmd->path, NULL, NULL)) {
        ocmd->header.header.status = -ENOENT;   // file doesn't exist
        luxSendKernel(ocmd);
        return;
    }

    // ensure this is a directory or a symbolic link to a directory
    uint8_t type = (entry.flags >> LXFS_DIR_TYPE_SHIFT) & LXFS_DIR_TYPE_MASK;
    if(type == LXFS_DIR_TYPE_SOFT_LINK) {
        if(lxfsReadBlock(mp, entry.block, mp->meta)) {
            ocmd->header.header.status = -EIO;
            luxSendKernel(ocmd);
            return;
        }

        memset(ocmd->path, 0, sizeof(ocmd->path));
        memcpy(ocmd->path, mp->meta, entry.size);
        if(ocmd->path[0] == '/') {
            memmove(ocmd->path, ocmd->path+1, entry.size-1);
            ocmd->path[entry.size-1] = 0;
        }

        strcpy(ocmd->abspath+1, ocmd->path);
        ocmd->abspath[0] = '/';

        lxfsOpendir(ocmd);
        return;
    }

    if(type != LXFS_DIR_TYPE_DIR) {
        ocmd->header.header.status = -ENOTDIR;
        luxSendKernel(ocmd);
        return;
    }

    // and ensure adequate permissions (execute)
    ocmd->header.header.status = 0;
    if((ocmd->uid == entry.owner) && !(entry.permissions & LXFS_PERMS_OWNER_X))
        ocmd->header.header.status = -EPERM;
    else if((ocmd->gid == entry.group) && !(entry.permissions & LXFS_PERMS_GROUP_X))
        ocmd->header.header.status = -EPERM;
    else if(!(entry.permissions & LXFS_PERMS_OTHER_X))
        ocmd->header.header.status = -EPERM;

    luxSendKernel(ocmd);
}

/* lxfsReaddir(): reads a directory entry from an lxfs volume
 * params: rcmd - read directory command message
 * returns: nothing, response relayed to vfs
 */

void lxfsReaddir(ReaddirCommand *rcmd) {
    rcmd->header.header.response = 1;
    rcmd->header.header.length = sizeof(ReaddirCommand);

    Mountpoint *mp = findMP(rcmd->device);
    if(!mp) {
        rcmd->header.header.status = -EIO;  // device doesn't exist
        luxSendKernel(rcmd);
        return;
    }

    LXFSDirectoryEntry entry;
    if(!lxfsFind(&entry, mp, rcmd->path, NULL, NULL)) {
        rcmd->header.header.status = -ENOENT;   // file doesn't exist
        luxSendKernel(rcmd);
        return;
    }

    // ensure this is a directory
    if(((entry.flags >> LXFS_DIR_TYPE_SHIFT) & LXFS_DIR_TYPE_MASK) != LXFS_DIR_TYPE_DIR) {
        rcmd->header.header.status = -ENOTDIR;
        luxSendKernel(rcmd);
        return;
    }

    // indexes 0 and 1 will always be '.' and '..'
    if(!rcmd->position) {
        strcpy(rcmd->entry.d_name, ".");
        rcmd->entry.d_ino = 1;
        rcmd->position++;
        rcmd->end = 0;
        rcmd->header.header.status = 0;
        luxSendKernel(rcmd);
        return;
    } else if(rcmd->position == 1) {
        strcpy(rcmd->entry.d_name, "..");
        rcmd->entry.d_ino = 2;
        rcmd->position++;
        rcmd->end = 0;
        rcmd->header.header.status = 0;
        luxSendKernel(rcmd);
        return;
    }

    // now simply iterate over the directory
    size_t position = 0;
    LXFSDirectoryEntry *dir = (LXFSDirectoryEntry *)((uintptr_t)mp->dataBuffer + sizeof(LXFSDirectoryHeader));
    uint64_t next = entry.block;

    if(lxfsReadBlock(mp, next, mp->dataBuffer)) {
        rcmd->header.header.status = -EIO;
        luxSendKernel(rcmd);
        return;
    }

    while(dir->entrySize) {
        next = lxfsReadNextBlock(mp, next, mp->dataBuffer);
        if(!next) {
            rcmd->header.header.status = -EIO;
            luxSendKernel(rcmd);
            return;
        }

        // read two blocks at a time for entries that cross boundaries
        if(next != LXFS_BLOCK_EOF)
            lxfsReadNextBlock(mp, next, mp->dataBuffer + mp->blockSizeBytes);
        
        dir = (LXFSDirectoryEntry *)((uintptr_t)mp->dataBuffer + sizeof(LXFSDirectoryHeader));
        off_t offset = sizeof(LXFSDirectoryHeader);

        while(offset < mp->blockSizeBytes) {
            if((dir->flags & LXFS_DIR_VALID) && (position == (rcmd->position-2))) {
                strcpy(rcmd->entry.d_name, (char *) dir->name);
                rcmd->entry.d_ino = dir->block;
                rcmd->position++;
                rcmd->end = 0;
                rcmd->header.header.status = 0;
                luxSendKernel(rcmd);
                return;
            }

            // skip over to the next entry
            position++;

            uint16_t oldSize = dir->entrySize;
            if(!oldSize) {
                rcmd->header.header.status = 0;
                rcmd->end = 1;
                luxSendKernel(rcmd);
                return;
            }

            dir = (LXFSDirectoryEntry *)((uintptr_t)dir + oldSize);
            offset += oldSize;

            if((offset >= mp->blockSizeBytes) && (next != LXFS_BLOCK_EOF)) {
                // copy the second block to the first block
                offset -= mp->blockSizeBytes;
                dir = (LXFSDirectoryEntry *)((uintptr_t)dir - mp->blockSizeBytes);
                memmove(mp->dataBuffer, mp->dataBuffer+mp->blockSizeBytes, mp->blockSizeBytes);

                if(!dir->entrySize) {
                    rcmd->header.header.status = 0;
                    rcmd->end = 1;
                    luxSendKernel(rcmd);
                    return;
                }

                // and read the next block
                next = lxfsReadNextBlock(mp, next, mp->dataBuffer);
                if(!next) {
                    rcmd->header.header.status = -EIO;
                    luxSendKernel(rcmd);
                    return;
                }
            }
        }
    }

    rcmd->header.header.status = 0;
    rcmd->end = 1;
    luxSendKernel(rcmd);
    return;
}