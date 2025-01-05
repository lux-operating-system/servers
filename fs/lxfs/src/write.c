/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024-25
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
#include <time.h>

/* lxfsWriteNew(): helper function to write to a new file
 * params: wcmd - write command message
 * params: mp - mountpoint
 * params: entry - directory entry for the file
 * params: metadata - file metadata block
 * returns: nothing, response relayed to kernel
 */

void lxfsWriteNew(RWCommand *wcmd, Mountpoint *mp, LXFSDirectoryEntry *entry, LXFSFileHeader *metadata) {
    // round up to block size
    uint64_t blockCount = (wcmd->length+mp->blockSizeBytes-1) / mp->blockSizeBytes;
    uint64_t block = lxfsAllocate(mp, blockCount);
    uint64_t first = block;
    if(!block) {
        wcmd->header.header.status = -ENOSPC;   /* out of space */
        luxSendKernel(wcmd);
        return;
    }

    uint64_t size = wcmd->length;
    uint64_t position = 0;
    while(size) {
        if(size > mp->blockSizeBytes) {
            memcpy(mp->dataBuffer, (const void *)((uintptr_t)wcmd->data + position), mp->blockSizeBytes);
            size -= mp->blockSizeBytes;
            position += mp->blockSizeBytes;
        } else {
            memcpy(mp->dataBuffer, (const void *)((uintptr_t)wcmd->data + position), size);
            size = 0;
        }

        block = lxfsWriteNextBlock(mp, block, mp->dataBuffer);
        if(!block) {
            wcmd->header.header.status = -EIO;
            luxSendKernel(wcmd);
            return;
        }
    }

    // update file metadata
    metadata->size = wcmd->length;
    if(lxfsWriteBlock(mp, entry->block, mp->meta)) {
        wcmd->header.header.status = -EIO;
        luxSendKernel(wcmd);
        return;
    }

    if(lxfsSetNextBlock(mp, entry->block, first)) {
        wcmd->header.header.status = -EIO;
        luxSendKernel(wcmd);
        return;
    }

    wcmd->header.header.status = wcmd->length;
    wcmd->position += wcmd->length;
    luxSendKernel(wcmd);
}

/* lxfsWrite(): writes to an opened file on an lxfs volume
 * params: wcmd - write command message
 * returns: nothing, response relayed to kernel
 */

void lxfsWrite(RWCommand *wcmd) {
    wcmd->header.header.response = 1;
    wcmd->header.header.length = sizeof(RWCommand);

    Mountpoint *mp = findMP(wcmd->device);
    if(!mp) {
        wcmd->header.header.status = -EIO;
        luxSendKernel(wcmd);
        return;
    }

    LXFSDirectoryEntry entry;
    if(!lxfsFind(&entry, mp, wcmd->path, NULL, NULL)) {
        wcmd->header.header.status = -ENOENT;
        luxSendKernel(wcmd);
        return;
    }

    uint64_t first = lxfsReadNextBlock(mp, entry.block, mp->meta);
    if(!first) {
        wcmd->header.header.status = -EIO;
        luxSendKernel(wcmd);
        return;
    }

    LXFSFileHeader *metadata = (LXFSFileHeader *) mp->meta;

    // the kernel will communicate O_APPEND by setting position to -1
    if(wcmd->position == -1)
        wcmd->position = metadata->size;

    /* TODO: handle case for padding with zeroes when position > actual size */
    if(wcmd->position > metadata->size) {
        luxLogf(KPRINT_LEVEL_ERROR, "TODO: position > file size, handle zero pad case\n");
        wcmd->header.header.status = -ENOSYS;   /* not implemented */
        luxSendKernel(wcmd);
        return;
    }

    // check if this is a new file
    if(first == LXFS_BLOCK_EOF) {
        lxfsWriteNew(wcmd, mp, &entry, metadata);
        return;
    }

    // here we're writing to an existing file
    uint64_t block = lxfsGetBlock(mp, first, wcmd->position);
    uint64_t prevBlock;
    if(wcmd->position >= mp->blockSizeBytes)
        prevBlock = lxfsGetBlock(mp, first, wcmd->position-mp->blockSizeBytes);
    else
        prevBlock = block;

    uint64_t size = wcmd->length;
    uint64_t position = 0;
    uint64_t tempPosition = wcmd->position % mp->blockSizeBytes;

    while(size && block && (block != LXFS_BLOCK_EOF)) {
        if(lxfsReadBlock(mp, block, mp->dataBuffer)) {
            wcmd->header.header.status = -EIO;
            luxSendKernel(wcmd);
            return;
        }

        if(size >= (mp->blockSizeBytes - tempPosition)) {
            memcpy((void *)((uintptr_t)mp->dataBuffer+tempPosition), (const void *)((uintptr_t)wcmd->data+position), mp->blockSizeBytes - tempPosition);
            size -= (mp->blockSizeBytes - tempPosition);
            position += (mp->blockSizeBytes - tempPosition);
            tempPosition = 0;
        } else {
            memcpy((void *)((uintptr_t)mp->dataBuffer+tempPosition), (const void *)((uintptr_t)wcmd->data+position), size);
            size = 0;
        }

        prevBlock = block;
        block = lxfsWriteNextBlock(mp, block, mp->dataBuffer);
        if(!block) {
            wcmd->header.header.status = -EIO;
            luxSendKernel(wcmd);
            return;
        }
    }

    if(size) {
        // allocate new blocks for the remaining bytes
        uint64_t blockCount = (size+mp->blockSizeBytes-1) / mp->blockSizeBytes;
        uint64_t newBlock = lxfsAllocate(mp, blockCount);
        uint64_t firstNewBlock = newBlock;
        if(!newBlock) {
            wcmd->header.header.status = -ENOSPC;   /* out of storage */
            luxSendKernel(wcmd);
            return;
        }

        while(size) {
            if(size > mp->blockSizeBytes) {
                memcpy(mp->dataBuffer, (const void *)((uintptr_t)wcmd->data + position), mp->blockSizeBytes);
                size -= mp->blockSizeBytes;
                position += mp->blockSizeBytes;
            } else {
                memcpy(mp->dataBuffer, (const void *)((uintptr_t)wcmd->data + position), size);
                size = 0;
            }

            newBlock = lxfsWriteNextBlock(mp, newBlock, mp->dataBuffer);
            if(!newBlock) {
                wcmd->header.header.status = -EIO;
                luxSendKernel(wcmd);
                return;
            }
        }

        // update the block list
        if(lxfsSetNextBlock(mp, prevBlock, firstNewBlock)) {
            wcmd->header.header.status = -EIO;
            luxSendKernel(wcmd);
            return;
        }
    }

    // and finally update the file metadata header
    metadata->size += wcmd->length;
    if(lxfsWriteBlock(mp, entry.block, mp->meta)) {
        wcmd->header.header.status = -EIO;
        luxSendKernel(wcmd);
        return;
    }

    // and update the timestamps
    time_t timestamp = time(NULL);
    uint64_t dirBlock;
    off_t dirOffset;
    if(!lxfsFind(&entry, mp, wcmd->path, &dirBlock, &dirOffset)) {
        wcmd->header.header.status = -EIO;
        luxSendKernel(wcmd);
        return;
    }

    LXFSDirectoryEntry *dir = (LXFSDirectoryEntry *)((uintptr_t)mp->dataBuffer + dirOffset);
    dir->accessTime = timestamp;
    dir->modTime = timestamp;

    uint64_t next = lxfsWriteNextBlock(mp, dirBlock, mp->dataBuffer);
    if(!next) {
        wcmd->header.header.status = -EIO;
        luxSendKernel(wcmd);
        return;
    }

    if((dirOffset + entry.entrySize) > mp->blockSizeBytes) {
        if(lxfsWriteBlock(mp, next, (const void *)((uintptr_t)mp->dataBuffer + mp->blockSizeBytes))) {
            wcmd->header.header.status = -EIO;
            luxSendKernel(wcmd);
            return;
        }
    }

    wcmd->header.header.status = wcmd->length;
    wcmd->position += wcmd->length;
    luxSendKernel(wcmd);
}