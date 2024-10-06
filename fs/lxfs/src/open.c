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
#include <fnctl.h>
#include <unistd.h>
#include <errno.h>

/* lxfsOpen(): opens a opened file on an lxfs volume
 * params: ocmd - open command message
 * returns: nothing, response relayed to vfs
 */

void lxfsOpen(OpenCommand *ocmd) {
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

    // ensure this is a file
    if(((entry.flags >> LXFS_DIR_TYPE_SHIFT) & LXFS_DIR_TYPE_MASK) != LXFS_DIR_TYPE_FILE) {
        ocmd->header.header.status = -EISDIR;
        luxSendDependency(ocmd);
        return;
    }

    // and that the requester has appropriate permissions
    ocmd->header.header.status = 0;
    if(ocmd->uid == entry.owner) {
        if((ocmd->flags & O_RDONLY) && !(entry.permissions & LXFS_PERMS_OWNER_R)) ocmd->header.header.status = -EACCES;
        if((ocmd->flags & O_WRONLY) && !(entry.permissions & LXFS_PERMS_OWNER_W)) ocmd->header.header.status = -EACCES;
    } else if(ocmd->gid == entry.group) {
        if((ocmd->flags & O_RDONLY) && !(entry.permissions & LXFS_PERMS_GROUP_R)) ocmd->header.header.status = -EACCES;
        if((ocmd->flags & O_WRONLY) && !(entry.permissions & LXFS_PERMS_GROUP_W)) ocmd->header.header.status = -EACCES;
    } else {
        if((ocmd->flags & O_RDONLY) && !(entry.permissions & LXFS_PERMS_OTHER_R)) ocmd->header.header.status = -EACCES;
        if((ocmd->flags & O_WRONLY) && !(entry.permissions & LXFS_PERMS_OTHER_W)) ocmd->header.header.status = -EACCES;
    }

    luxSendDependency(ocmd);
}