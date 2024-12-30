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
    return -ENOSYS;     /* TODO */
}
