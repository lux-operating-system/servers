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
#include <errno.h>

/* lxfsCreate(): creates a file or directory on the lxfs volume
 * params: dest - destination buffer to store directory entry
 * params: mp - mountpoint
 * params: path - path to create
 * params: mode - file mode bits
 * params: uid - user ID of the owner
 * params: gid - group ID of the file
 * returns: zero on success, negative errno error code on fail
 */

int lxfsCreate(LXFSDirectoryEntry *dest, Mountpoint *mp, const char *path,
               mode_t mode, uid_t uid, gid_t gid) {
    // get the parent directory
    LXFSDirectoryEntry parent;
    int depth = pathDepth(path);
    if(depth <= 1) {
        if(!lxfsFind(&parent, mp, "/")) return -EIO;
    } else {
        char *parentPath = strdup(path);
        if(!parentPath) return -1 * errno;
        char *last = strrchr(path, '/');
        if(!last) {
            free(parentPath);
            return -ENOENT;
        }

        *last = 0;

        if(!lxfsFind(&parent, mp, parentPath)) {
            free(parentPath);
            return -ENOENT;
        }

        free(parentPath);
    }

    if(!pathComponent((char *) dest->name, path, depth-1))
        return -ENOENT;

    // ensure the parent is a directory
    if((parent.flags >> LXFS_DIR_TYPE_SHIFT) != LXFS_DIR_TYPE_DIR)
        return -ENOTDIR;
    
    // and that we have write permission
    if(uid == parent.owner) {
        if(!(parent.permissions & LXFS_PERMS_OWNER_W)) return -EACCES;
    } else if(gid == parent.group) {
        if(!(parent.permissions & LXFS_PERMS_GROUP_W)) return -EACCES;
    } else {
        if(!(parent.permissions & LXFS_PERMS_OTHER_W)) return -EACCES;
    }

    dest->entrySize = sizeof(LXFSDirectoryEntry) + strlen((const char *) dest->name) - 511;

    luxLogf(KPRINT_LEVEL_DEBUG, "attempt to create entry of size %d on parent starting block %d\n", dest->entrySize, parent.block);
    return -ENOSYS;     /* TODO: stub */
}
