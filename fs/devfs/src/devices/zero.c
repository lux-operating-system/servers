/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * devfs: Microkernel server implementing the /dev file system
 */

/* Implementation of /dev/zero */

#include <devfs/devfs.h>
#include <string.h>

ssize_t zeroIOHandler(int write, const char *name, off_t *position, void *buffer, size_t len) {
    if(!write) memset(buffer, 0, len);

    *position += len;
    return len;
}
