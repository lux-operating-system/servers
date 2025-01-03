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
 * returns: maximum depth of the path, zero for root directory, ONE-BASED elsewhere
 */

int pathDepth(const char *path) {
    if(!path || !strlen(path) || strlen(path) == 1) return 0;

    int c = 1;
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

    if(depth <= 1) {
        // special case for files on the root directory
        return strcpy(dest, path);
    }

    int c = 0;
    int len = 0;
    for(int i = 0; i < strlen(path); i++) {
        if(path[i] == '/') c++;
        if(c >= n) {
            while(path[i] == '/') i++;
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
 * params: blockPtr - pointer to store starting block
 * params: offPtr - pointer to store starting offset within the block
 * returns: pointer to destination on success, NULL on fail
 */

LXFSDirectoryEntry *lxfsFind(LXFSDirectoryEntry *dest, Mountpoint *mp, const char *path, uint64_t *blockPtr, off_t *offPtr) {
    // special case for the root directory because it does not have a true
    // directory entry within the file system itself
    if((strlen(path) == 1) && (path[0] == '/')) {
        if(lxfsReadBlock(mp, mp->root, mp->dataBuffer)) return NULL;

        LXFSDirectoryHeader *root = (LXFSDirectoryHeader *) mp->dataBuffer;
        dest->size = 1;
        dest->accessTime = root->accessTime;
        dest->createTime = root->createTime;
        dest->modTime = root->modTime;
        dest->name[0] = 0;
        dest->flags = LXFS_DIR_VALID | (LXFS_DIR_TYPE_DIR << LXFS_DIR_TYPE_SHIFT);
        dest->block = mp->root;

        // root directory is owned by root:root and its mode is rwxr-xr-x
        dest->owner = 0;
        dest->group = 0;
        dest->permissions = LXFS_PERMS_OWNER_R | LXFS_PERMS_OWNER_W | LXFS_PERMS_OWNER_X | LXFS_PERMS_GROUP_R | LXFS_PERMS_GROUP_X | LXFS_PERMS_OTHER_R | LXFS_PERMS_OTHER_X;

        dest->entrySize = sizeof(LXFSDirectoryEntry) - 510; // 2 bytes path
        strcpy((char *) dest->name, "/");

        return dest;
    }

    // for everything else we will need to traverse the file system starting
    // at the root directory
    LXFSDirectoryEntry *dir;
    uint64_t prev;
    uint64_t next = mp->root;
    int depth = pathDepth(path);
    char component[MAX_FILE_PATH];

    int i = 0;

traverse:
    while(i < depth) {
        // iterate over each component in the path and search for it in the directory
        if(!pathComponent(component, path, i)) return NULL;

        prev = next;
        next = lxfsReadNextBlock(mp, next, mp->dataBuffer);
        if(!next) return NULL;
        
        // read two blocks at a time for entries that cross boundaries
        if(next != LXFS_BLOCK_EOF)
            lxfsReadNextBlock(mp, next, mp->dataBuffer + mp->blockSizeBytes);

        dir = (LXFSDirectoryEntry *)((uintptr_t)mp->dataBuffer + sizeof(LXFSDirectoryHeader));
        off_t offset = sizeof(LXFSDirectoryHeader);

        while(offset < mp->blockSizeBytes) {
            if((dir->flags & LXFS_DIR_VALID) && !strcmp((const char *) dir->name, component)) {
                if(i == depth-1) {
                    // found the file we're looking for
                    if(blockPtr) *blockPtr = prev;
                    if(offPtr) *offPtr = offset;
                    return (LXFSDirectoryEntry *) memcpy(dest, dir, dir->entrySize);
                } else {
                    // found a parent component, ensure it is a directory
                    i++;

                    if(((dir->flags >> LXFS_DIR_TYPE_SHIFT) & LXFS_DIR_TYPE_MASK) != LXFS_DIR_TYPE_DIR)
                        return NULL;

                    // and go back to the beginning of the loop but searching for this component
                    next = dir->block;
                    goto traverse;
                }
            }
            
            // advance to the next entry
            uint16_t oldSize = dir->entrySize;
            dir = (LXFSDirectoryEntry *)((uintptr_t)dir + oldSize);
            offset += oldSize;

            if(!dir->entrySize) {
                // file doesn't exist
                return NULL;
            }

            if((offset >= mp->blockSizeBytes) && (next != LXFS_BLOCK_EOF)) {
                // copy the second block to the first block
                offset -= mp->blockSizeBytes;
                dir = (LXFSDirectoryEntry *)((uintptr_t)dir - mp->blockSizeBytes);
                memmove(mp->dataBuffer, mp->dataBuffer+mp->blockSizeBytes, mp->blockSizeBytes);

                if(!dir->entrySize) return NULL;

                // and read the next block
                next = lxfsReadNextBlock(mp, next, mp->dataBuffer);
                if(!next) return NULL;
            }
        }
    }

    return NULL;
}