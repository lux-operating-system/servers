/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024-25
 * 
 * lxfs: Driver for the lxfs file system
 */

#include <liblux/liblux.h>
#include <lxfs/lxfs.h>
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
