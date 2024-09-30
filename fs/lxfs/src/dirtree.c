/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * lxfs: Driver for the lxfs file system
 */

#include <lxfs/lxfs.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

/* pathDepth(): helper function to calculate the depth of a path
 * params: path - full qualified path
 * returns: maximum depth of the path
 */

int pathDepth(const char *path) {
    if(!path || !strlen(path) || strlen(path) == 1) return 0;

    int c = 0;
    for(int i = 0; i < strlen(path); i++) {
        if(path[i] == '/') c++;
    }

    return c;
}

/* pathComponent(): helper function to return the nth component of a path
 * params: dest - destination buffer
 * params: path - full qualified path
 * params: n - component number
 * returns: pointer to destination on success, NULL on fail
 */

char *pathComponent(char *dest, const char *path, int n) {
    int depth = pathDepth(path);
    if(n > depth) return NULL;

    int c = 0;
    int len = 0;
    for(int i = 0; i < strlen(path); i++) {
        if(path[i] == '/') c++;
        if(c > n) {
            i++;
            while(path[i] && (path[i] != '/')) {
                dest[len] = path[i];
                len++;
                i++;
            }

            dest[len] = 0;
            return dest;
        }
    }

    return NULL;
}

/* lxfsFind(): finds the directory entry associated with a file
 * params: dest - destination buffer to store the directory entry
 * params: mp - lxfs mountpoint
 * params: path - full qualified path
 * returns: pointer to destination on success, NULL on fail
 */

LXFSDirectoryEntry *lxfsFind(LXFSDirectoryEntry *dest, Mountpoint *mp, const char *path) {
    return NULL;    // stub
}