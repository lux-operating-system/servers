/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * lxfs: Driver for the lxfs file system
 */

#include <errno.h>
#include <liblux/liblux.h>
#include <lxfs/lxfs.h>
#include <sys/types.h>
#include <sys/stat.h>

void lxfsStat(StatCommand *cmd) {
    cmd->header.header.response = 1;
    cmd->header.header.length = sizeof(StatCommand);
    
    // find the mountpoint
    Mountpoint *mp = findMP(cmd->source);
    if(!mp) {
        cmd->header.header.status = -EIO;
        luxSendKernel(cmd);
        return;
    }

    // and the file entry
    LXFSDirectoryEntry entry;
    if(!lxfsFind(&entry, mp, cmd->path, NULL, NULL)) {
        cmd->header.header.status = -ENOENT;
        luxSendKernel(cmd);
        return;
    }

    // use the file entry to read metadata as well
    uint64_t first = lxfsReadNextBlock(mp, entry.block, mp->meta);
    if(!first) {
        cmd->header.header.status = -EIO;
        luxSendKernel(cmd);
        return;
    }

    // now construct the stat structure
    cmd->buffer.st_atime = entry.accessTime;
    cmd->buffer.st_mtime = entry.modTime;
    cmd->buffer.st_ctime = entry.createTime;
    cmd->buffer.st_blksize = mp->blockSizeBytes;
    cmd->buffer.st_uid = entry.owner;
    cmd->buffer.st_gid = entry.group;
    cmd->buffer.st_dev = mp->fd;
    cmd->buffer.st_rdev = mp->fd;
    cmd->buffer.st_ino = first;
    
    // parse the mode
    uint8_t type = (entry.flags >> LXFS_DIR_TYPE_SHIFT) & LXFS_DIR_TYPE_MASK;
    switch(type) {
    case LXFS_DIR_TYPE_DIR:
        LXFSDirectoryHeader *dirMeta = (LXFSDirectoryHeader *) mp->meta;
        cmd->buffer.st_mode = S_IFDIR;
        cmd->buffer.st_size = dirMeta->sizeBytes;
        cmd->buffer.st_blocks = (dirMeta->sizeBytes+mp->blockSizeBytes-1) / mp->blockSizeBytes;
        cmd->buffer.st_nlink = 1;
        cmd->buffer.st_atime = dirMeta->accessTime;
        cmd->buffer.st_mtime = dirMeta->modTime;
        cmd->buffer.st_ctime = dirMeta->createTime;
        break;
    case LXFS_DIR_TYPE_SOFT_LINK:
        cmd->buffer.st_mode = S_IFLNK;
        cmd->buffer.st_nlink = 1;
        cmd->buffer.st_size = entry.size;
        cmd->buffer.st_blocks = (entry.size+mp->blockSizeBytes-1) / mp->blockSizeBytes;
        break;
    case LXFS_DIR_TYPE_FILE:
    case LXFS_DIR_TYPE_HARD_LINK:
    default:
        LXFSFileHeader *fileMeta = (LXFSFileHeader *) mp->meta;
        cmd->buffer.st_mode = S_IFREG;
        cmd->buffer.st_blocks = (fileMeta->size+mp->blockSizeBytes-1) / mp->blockSizeBytes;
        cmd->buffer.st_size = fileMeta->size;
        cmd->buffer.st_nlink = fileMeta->refCount;
    }

    if(entry.permissions & LXFS_PERMS_OWNER_R) cmd->buffer.st_mode |= S_IRUSR;
    if(entry.permissions & LXFS_PERMS_OWNER_W) cmd->buffer.st_mode |= S_IWUSR;
    if(entry.permissions & LXFS_PERMS_OWNER_X) cmd->buffer.st_mode |= S_IXUSR;
    
    if(entry.permissions & LXFS_PERMS_GROUP_R) cmd->buffer.st_mode |= S_IRGRP;
    if(entry.permissions & LXFS_PERMS_GROUP_W) cmd->buffer.st_mode |= S_IWGRP;
    if(entry.permissions & LXFS_PERMS_GROUP_X) cmd->buffer.st_mode |= S_IXGRP;
    
    if(entry.permissions & LXFS_PERMS_OTHER_R) cmd->buffer.st_mode |= S_IROTH;
    if(entry.permissions & LXFS_PERMS_OTHER_W) cmd->buffer.st_mode |= S_IWOTH;
    if(entry.permissions & LXFS_PERMS_OTHER_X) cmd->buffer.st_mode |= S_IXOTH;

    // and we're done, relay the response
    cmd->header.header.status = 0;
    luxSendKernel(cmd);
}