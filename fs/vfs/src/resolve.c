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

/* clean(): helper function to clean up a path
 * params: path - path to be cleaned, the string will be directly modified
 * params: pointer to path
 */

static char *clean(char *path) {
    if(!strlen(path)) {
        path[0] = '/';
        path[1] = 0;
        return path;
    }

    if(strlen(path) == 1) return path;  // root will never need to be cleaned

    // first try to get rid of excessive slashes
    for(int i = 0; i < strlen(path); i++) {
        if((path[i] == '/') && (i < strlen(path)-1) && (path[i+1] == '/')) {
            memmove(&path[i], &path[i+1], strlen(&path[i])+1);
            continue;
        }
    }

    // if the last character is a slash, remove it except for the root dir
    if(strlen(path) == 1) return path;
    while(path[strlen(path)-1] == '/') path[strlen(path)-1] = 0;

    // prevent '..' from doing anything on the root dir
    if(!strcmp(path, "/..")) {
        path[1] = 0;
        return path;
    }

    if(!memcmp(path, "/../", 4)) {
        memmove(path, path+3, strlen(path+3)+1);
        return clean(path);
    }

    // parse '../' to reflect the parent directory
    for(int i = 0; i < strlen(path); i++) {
        if((path[i] == '.') && (path[i+1] == '.') && ((path[i+2] == '/') || (!path[i+2]))) {
            // find the parent
            int parent = i-2;
            for(; parent > 0; parent--) {
                if(path[parent] == '/') break;
            }

            parent++;
            memmove(&path[parent], &path[i+3], strlen(&path[i+3])+1);
        }
    }

    // remove './' because it refers to the self directory
    for(int i = 0; i < strlen(path); i++) {
        if((path[i] == '.') && ((path[i+1] == '/') || (!path[i+1]))) {
            memmove(&path[i], &path[i+2], strlen(&path[i+2])+1);
            continue;
        }
    }

    // and finally remove any trailing slashes left while processing the string
    if(strlen(path) == 1) return path;
    while(path[strlen(path)-1] == '/') path[strlen(path)-1] = 0;

    return path;
}

/* resolve(): resolves a path relative to a mountpoint
 * params: buffer - destination to store the resolved path
 * params: type - buffer to store file system type
 * params: path - source absolute path
 * returns: pointer to resolved path on success, NULL on fail
 */

char *resolve(char *buffer, char *type, char *source, char *path) {
    // iterate over the mountpoints to determine which device this path is
    // located on
    if(!mpCount) return NULL;

    clean(path);

    for(int i = 0; i < mpCount; i++) {
        size_t mplen = strlen(mps[i].path);
        if(!memcmp(path, mps[i].path, mplen)) {
            strcpy(type, mps[i].type);
            strcpy(source, mps[i].device);
            if(!strcmp(path, mps[i].path)) return strcpy(buffer, "/");
            else return (char *) memmove(buffer, path+mplen, strlen(path+mplen)+1);
        }
    }

    return NULL;
}