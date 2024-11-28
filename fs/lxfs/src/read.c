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

/* lxfsRead(): reads from an opened file on an lxfs volume
 * params: rcmd - read command message
 * returns: nothing, response relayed to vfs
 */

void lxfsRead(RWCommand *rcmd) {
    rcmd->header.header.response = 1;
    rcmd->header.header.length = sizeof(RWCommand);

    // get the mountpoint
    Mountpoint *mp = findMP(rcmd->device);
    if(!mp) {
        rcmd->header.header.status = -EIO;
        luxSendDependency(rcmd);
        return;
    }

    // and the file entry
    LXFSDirectoryEntry entry;
    if(!lxfsFind(&entry, mp, rcmd->path)) {
        rcmd->header.header.status = -ENOENT;
        luxSendDependency(rcmd);
        return;
    }

    // use the file entry to read metadata as well as find the first file block
    uint64_t first = lxfsReadNextBlock(mp, entry.block, mp->meta);
    if(!first) {
        rcmd->header.header.status = -EIO;
        luxSendDependency(rcmd);
        return;
    }

    LXFSFileHeader *metadata = (LXFSFileHeader *) mp->meta;

    // input validation
    if(rcmd->position >= metadata->size) {
        rcmd->header.header.status = -EOVERFLOW;
        luxSendDependency(rcmd);
        return;
    }

    size_t truelen;
    if((rcmd->position + rcmd->length) > metadata->size)
        truelen = metadata->size - rcmd->position;
    else
        truelen = rcmd->length;
    
    RWCommand *res = calloc(1, sizeof(RWCommand) + truelen);
    if(!res) {
        rcmd->header.header.status = -ENOMEM;
        luxSendDependency(rcmd);
        return;
    }

    // copy the header
    memcpy(res, rcmd, sizeof(RWCommand));

    // now calculate which block to start from and offset into the firsty block
    uint64_t startBlock = rcmd->position / mp->blockSizeBytes;
    uint64_t startOffset = rcmd->position % mp->blockSizeBytes;
    uint64_t block = first;

    // find the starting block
    while(startBlock) {
        block = lxfsNextBlock(mp, block);
        if(!block) {
            free(res);
            rcmd->header.header.status = -EIO;
            luxSendDependency(rcmd);
            return;
        }

        startBlock--;
    }

    // and begin - we will use separate counters for this, because even though
    // we have an ideal "true length" to read, we cannot guarantee that we will
    // actually read that many bytes due to things like I/O errors, file system
    // corruption, missing blocks, etc
    size_t readCount = 0;
    size_t remaining = truelen;
    while(readCount < truelen) {
        size_t s;

        // here and ONLY here we're allowed to break out of the loop without
        // throwing errors, provided we've read at least some of the file
        if(block == LXFS_BLOCK_EOF) break;
        block = lxfsReadNextBlock(mp, block, mp->dataBuffer);
        if(!block) break;

        if(!readCount) {
            // special case for starting block
            if(remaining >= (mp->blockSizeBytes - startOffset)) s = mp->blockSizeBytes - startOffset;
            else s = remaining;

            memcpy((void *)((uintptr_t)res->data + readCount), mp->dataBuffer+startOffset, s);
        } else {
            // for all other blocks
            if(remaining > mp->blockSizeBytes) s = mp->blockSizeBytes;
            else s = remaining;

            memcpy((void *)((uintptr_t)res->data + readCount), mp->dataBuffer, s);
        }

        readCount += s;
        remaining -= s;
    }

    // appropriately update the file descriptor position and status flags
    if(readCount) {
        res->position += readCount;
        res->length = readCount;
        res->header.header.status = readCount;
        res->header.header.length += readCount;
    } else {
        res->header.header.status = -EIO;
    }

    luxSendDependency(res);
    free(res);
}