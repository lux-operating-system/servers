/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * devfs: Microkernel server implementing the /dev file system
 */

#include <devfs/devfs.h>
#include <liblux/liblux.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

/* copyPathDepth(): copies a path up to depth n
 * params: dst - destination buffer
 * params: path - path
 * params: n - depth
 * returns: pointer to destination on success, NULL on error
 */

char *copyPathDepth(char *dst, const char *path, int n) {
    int depth = 0;
    for(int i = 0; i < strlen(path); i++) {
        if(path[i] == '/') depth++;
        if(depth > n) return dst;

        dst[i] = path[i];
    }

    return dst;
}

/* createDirectories(): creates the directory hierarchy if necessary
 * params: name - name of the device file
 * returns: zero on success
 */

int createDirectories(const char *name) {
    // first check if this is even necessary at all
    int depth = 0;
    for(int i = 0; i < strlen(name); i++) {
        if(name[i] == '/') depth++;
    }

    if(depth <= 1) return 0;

    char *dirs = malloc(strlen(name)+1);
    if(!dirs) return -1;

    DeviceFile *entry;

    // default directory permissions are rwxr-xr-x
    struct stat dirstat;
    memset(&dirstat, 0, sizeof(struct stat));
    dirstat.st_mode = S_IFDIR | S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
    dirstat.st_size = 1;

    for(int i = 0; i < depth-1; i++) {
        copyPathDepth(dirs, name, i+1);
        entry = findDevice(dirs);
        if(!entry) {
            // create the directory if non-existent
            if(createDevice(dirs, NULL, &dirstat)) return -1;
        } else {
            // ensure this is a directory
            if((entry->status.st_mode & S_IFMT) != S_IFDIR) return -1;

            entry->status.st_size++;
        }
    }

    return 0;
}

/* createDevice(): creates a device file
 * params: name - name of the device file
 * params: handler - I/O handler for read/write operations on this device
 * params: status - file status structure
 * returns: zero on success
 */

int createDevice(const char *name, ssize_t (*handler)(int, const char *, off_t *, void *, size_t), struct stat *status) {
    if(deviceCount >= MAX_DEVICES) return -1;
    if(createDirectories(name)) return -1;

    devices[deviceCount].name = malloc(MAX_FILE_PATH);
    if(!devices[deviceCount].name) return -1;

    devices[deviceCount].ioHandler = handler;
    memcpy(&devices[deviceCount].status, status, sizeof(struct stat));
    strcpy(devices[deviceCount].name, name);

    devices[deviceCount].status.st_ino = deviceCount + 1;   // fake inode number
    devices[deviceCount].status.st_ctime = time(NULL);
    devices[deviceCount].status.st_mtime = devices[deviceCount].status.st_ctime;
    devices[deviceCount].status.st_atime = devices[deviceCount].status.st_ctime;
    devices[deviceCount].external = 0;

    //luxLogf(KPRINT_LEVEL_DEBUG, "created %s '/dev%s'\n", mode, name);

    deviceCount++;
    return 0;
}

/* findDevice(): finds the device file associated with a name
 * params: name - name of the device
 * returns: pointer to device file structure, NULL on error
 */

DeviceFile *findDevice(const char *name) {
    if(!deviceCount) return NULL;

    for(int i = 0; i < deviceCount; i++) {
        if(!strcmp(devices[i].name, name)) return &devices[i];
    }

    return NULL;
}
