/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * devfs: Microkernel server implementing the /dev file system
 */

/* Implementation of /dev/null */

#include <devfs/devfs.h>

ssize_t nullIOHandler(int write, const char *name, off_t *position, void *buffer, size_t len) {
    // ignore all reads and writes
    *position += len;
    return len;
}
