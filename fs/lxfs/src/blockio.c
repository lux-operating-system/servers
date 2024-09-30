/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * lxfs: Driver for the lxfs file system
 */

#include <lxfs/lxfs.h>
#include <unistd.h>

/* lxfsReadBlock(): reads a block on a mounted lxfs partition
 * params: mp - mountpoint
 * params: block - block number
 * params: buffer - buffer to read into
 * returns: zero on success
 */

int lxfsReadBlock(Mountpoint *mp, uint64_t block, void *buffer) {
    lseek(mp->fd, block * mp->blockSizeBytes, SEEK_SET);
    return !(read(mp->fd, buffer, mp->blockSizeBytes) == mp->blockSizeBytes);
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
