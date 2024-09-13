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
