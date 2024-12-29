/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * lxfs: Driver for the lxfs file system
 */

#include <lxfs/lxfs.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

/* lxfsReadBlock(): reads a block on a mounted lxfs partition
 * params: mp - mountpoint
 * params: block - block number
 * params: buffer - buffer to read into
 * returns: zero on success
 */

int lxfsReadBlock(Mountpoint *mp, uint64_t block, void *buffer) {
    // check if the block is already in the cache
    uint64_t tag = block / CACHE_SIZE;
    uint64_t index = block % CACHE_SIZE;

    if(mp->cache[index].valid && (mp->cache[index].tag == tag)) {
        memcpy(buffer, mp->cache[index].data, mp->blockSizeBytes);
        return 0;
    }

    // else bring it into the cache
    if(mp->cache[index].valid && mp->cache[index].dirty) {
        // TODO: flush cache
        luxLogf(KPRINT_LEVEL_WARNING, "TODO: flush file system cache in read()\n");
        return -1;
    }

    mp->cache[index].valid = 1;
    mp->cache[index].dirty = 0;
    mp->cache[index].tag = tag;

    if(!mp->cache[index].data) mp->cache[index].data = malloc(mp->blockSizeBytes);
    if(!mp->cache[index].data) return -1;

    lseek(mp->fd, block * mp->blockSizeBytes, SEEK_SET);
    ssize_t s = read(mp->fd, mp->cache[index].data, mp->blockSizeBytes);
    if(s != mp->blockSizeBytes) {
        mp->cache[index].valid = 0;
        return 1;
    }

    memcpy(buffer, mp->cache[index].data, mp->blockSizeBytes);
    return 0;
}

/* lxfsWriteBlock(): writes a block to a mounted lxfs partition
 * params: mp - mountpoint
 * params: block - block number
 * params: buffer - buffer to write from
 * returns: zero on success
 */

int lxfsWriteBlock(Mountpoint *mp, uint64_t block, const void *buffer) {
    lseek(mp->fd, block * mp->blockSizeBytes, SEEK_SET);
    return !(write(mp->fd, buffer, mp->blockSizeBytes) == mp->blockSizeBytes);
}

/* lxfsNextBlock(): returns the next block in a chain of blocks
 * params: mp - mountpoint
 * params: block - current block number
 * returns: next block number, zero on fail
 */

uint64_t lxfsNextBlock(Mountpoint *mp, uint64_t block) {
    // read the relevant part of the block table
    uint64_t tableBlock = block / (mp->blockSizeBytes / 8);
    tableBlock += 33;   // the first 33 blocks are reserved
    uint64_t tableIndex = block % (mp->blockSizeBytes / 8);

    if(lxfsReadBlock(mp, tableBlock, mp->blockTableBuffer)) return 0;

    uint64_t *data = (uint64_t *) mp->blockTableBuffer;
    return data[tableIndex];
}

/* lxfsReadNextBlock(): reads a block and returns the next block in its chain
 * params: mp - mountpoint
 * params: block - block number
 * params: buffer - buffer to read into
 * returns: next block number, zero on fail
 */

uint64_t lxfsReadNextBlock(Mountpoint *mp, uint64_t block, void *buffer) {
    if(lxfsReadBlock(mp, block, buffer)) return 0;
    return lxfsNextBlock(mp, block);
}

/* lxfsWriteNextBlock(): writes a block and returns the next block in its chain
 * params: mp - mountpoint
 * params: block - block number
 * params: buffer - buffer to write from
 * returns: next block number, zero on fail
 */

uint64_t lxfsWriteNextBlock(Mountpoint *mp, uint64_t block, const void *buffer) {
    if(lxfsWriteBlock(mp, block, buffer)) return 0;
    return lxfsNextBlock(mp, block);
}

/* lxfsFindFreeBlock(): finds a free block on the LXFS volume
 * params: mp - mountpoint
 * params: index - zero-based index of free blocks to return
 * i.e. guarantee the volume has at least (index) free blocks
 * returns: block number, zero if no space available
 */

uint64_t lxfsFindFreeBlock(Mountpoint *mp, uint64_t index) {
    uint64_t block;
    uint64_t current = 0;
    for(uint64_t i = 33; i < mp->volumeSize; i++) {
        block = lxfsNextBlock(mp, i);
        if(block == LXFS_BLOCK_FREE) current++;
        if(current > index) return i;
    }

    return 0;
}
