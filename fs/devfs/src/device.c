/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * devfs: Microkernel server implementing the /dev file system
 */

#include <devfs/devfs.h>
#include <liblux/liblux.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

/* createDevice(): creates a device file
 * params: name - name of the device file
 * params: handler - I/O handler for read/write operations on this device
 * params: status - file status structure
 * returns: zero on success
 */

int createDevice(const char *name, void (*handler)(int, off_t, size_t), struct stat *status) {
    if(deviceCount >= MAX_DEVICES) return -1;

    char *mode = NULL;
    switch(status->st_mode & S_IFMT) {
    case S_IFBLK:
        mode = "block special file";
        break;
    case S_IFCHR:
        mode = "character special file";
        break;
    case S_IFDIR:
        mode = "directory";
        break;
    case S_IFIFO:
        mode = "named pipe";
        break;
    case S_IFREG:
        mode = "regular file";
        break;
    case S_IFLNK:
        mode = "symlink";
        break;
    default:
        return -1;
    }

    devices[deviceCount].ioHandler = handler;
    memcpy(&devices[deviceCount].status, status, sizeof(struct stat));
    strcpy(devices[deviceCount].name, name);

    devices[deviceCount].status.st_ino = deviceCount + 1;   // fake inode number

    luxLogf(KPRINT_LEVEL_DEBUG, "created %s '/dev%s'\n", mode, name);

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
