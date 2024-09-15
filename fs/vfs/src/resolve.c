/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * vfs: Microkernel server implementing a virtual file system
 */

#include <vfs/vfs.h>
#include <liblux/liblux.h>
#include <vfs.h>
#include <string.h>

/* resolve(): resolves a path relative to a mountpoint
 * params: buffer - destination to store the resolved path
 * params: src - source absolute path
 * returns: pointer to resolved path on success, NULL on fail
 */

char *resolve(char *buffer, const char *src) {
    // iterate over the mountpoints to determine which device this path is
    // located on
    if(!mpCount) return NULL;

    for(int i = 0; i < mpCount; i++) {
        size_t mplen = strlen(mps[i].path);
        if(!memcmp(src, mps[i].path, mplen)) return strcpy(buffer, src+mplen);
    }

    return NULL;
}