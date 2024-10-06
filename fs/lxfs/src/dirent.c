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
    ocmd->header.header.length = sizeof(OpenCommand);

    Mountpoint *mp = findMP(ocmd->device);
    if(!mp) {
        ocmd->header.header.status = -EIO;  // device doesn't exist
        luxSendDependency(ocmd);
        return;
    }

    LXFSDirectoryEntry entry;
    if(!lxfsFind(&entry, mp, ocmd->path)) {
        ocmd->header.header.status = -ENOENT;   // file doesn't exist
        luxSendDependency(ocmd);
        return;
    }

    // ensure this is a directory
    if(((entry.flags >> LXFS_DIR_TYPE_SHIFT) & LXFS_DIR_TYPE_MASK) != LXFS_DIR_TYPE_DIR) {
        ocmd->header.header.status = -EISDIR;
        luxSendDependency(ocmd);
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

    luxSendDependency(ocmd);
}