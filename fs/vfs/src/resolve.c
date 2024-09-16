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
 * params: type - buffer to store file system type
 * params: path - source absolute path
 * returns: pointer to resolved path on success, NULL on fail
 */

char *resolve(char *buffer, char *type, char *source, const char *path) {
    // iterate over the mountpoints to determine which device this path is
    // located on
    if(!mpCount) return NULL;

    for(int i = 0; i < mpCount; i++) {
        size_t mplen = strlen(mps[i].path);
        if(!memcmp(path, mps[i].path, mplen)) {
            strcpy(type, mps[i].type);
            strcpy(source, mps[i].device);
            return strcpy(buffer, path+mplen);
        }
    }

    return NULL;
}