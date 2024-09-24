/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * devfs: Microkernel server implementing the /dev file system
 */

#pragma once

#include <liblux/liblux.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAX_DEVICES             1024
#define MAX_DRIVERS             1024

#define DEVFS_CHR_PERMS         (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH | S_IFCHR)

/* generic structure that will be used to maintain a list of files on /dev */

typedef struct {
    char *name;         // file name
    struct stat status;
    ssize_t (*ioHandler)(int, const char *, off_t *, void *, size_t);
    int external;       // external device driver
    int socket;         // socket descriptor for external driver
    int handleOpen;     // external driver overrides default open() and close()
    char *server;       // server name of the external driver
} DeviceFile;

extern void (*dispatchTable[])(SyscallHeader *, SyscallHeader *);
extern DeviceFile *devices;
extern int deviceCount;

int createDevice(const char *, ssize_t (*)(int, const char *, off_t *, void *, size_t), struct stat *);
DeviceFile *findDevice(const char *);
void driverInit();
void driverHandle();
void driverRead(RWCommand *, DeviceFile *);
void driverWrite(RWCommand *, DeviceFile *);

ssize_t nullIOHandler(int, const char *, off_t *, void *, size_t);
ssize_t zeroIOHandler(int, const char *, off_t *, void *, size_t);
ssize_t randomIOHandler(int, const char *, off_t *, void *, size_t);
