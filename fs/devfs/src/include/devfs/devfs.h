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

#define MAX_DEVICES             256

#define DEVFS_CHR_PERMS         (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH | S_IFCHR)

/* generic structure that will be used to maintain a list of files on /dev */

typedef struct {
    char name[MAX_FILE_PATH];
    struct stat status;
    void (*ioHandler)(int, off_t, size_t);
} DeviceFile;

extern void (*dispatchTable[])(SyscallHeader *, SyscallHeader *);
extern DeviceFile *devices;
extern int deviceCount;

int createDevice(const char *, void (*)(int, off_t, size_t), struct stat *);
DeviceFile *findDevice(const char *);