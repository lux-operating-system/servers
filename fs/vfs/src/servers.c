/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * vfs: Microkernel server implementing a virtual file system
 */

#include <string.h>
#include <liblux/liblux.h>
#include <vfs.h>
#include <vfs/vfs.h>

/* findFSServer(): returns the socket descriptor of a file system driver
 * params: type - file system type
 * returns: socket descriptor, -1 if no driver is loaded
 */

int findFSServer(const char *type) {
    if(!serverCount) return -1;

    for(int i = 0; i < serverCount; i++) {
        if(!strcmp(type, servers[i].type)) return servers[i].socket;
    }

    return -1;
}

/* findMountpoint(): returns the socket descriptor of a fs driver from a monutpoint
 * params: mp - mounted device path
 * returns: socket descriptor, -1 if no driver is loaded or no such mountpoint
 */

int findMountpoint(const char *mp) {
    if(!serverCount) return -1;

    for(int i = 0; i < serverCount; i++) {
        if(!strcmp(mp, mps[i].device)) return findFSServer(mps[i].type);
    }

    return -1;
}