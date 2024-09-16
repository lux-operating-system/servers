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

/* generic structure that will be used to maintain a list of files on /dev */

typedef struct {
    char name[MAX_FILE_PATH];
    struct stat status;
    void (*ioHandler)(int, off_t, size_t);
} DeviceFile;

extern void (*dispatchTable[])(SyscallHeader *, SyscallHeader *);
extern DeviceFile *devices;
extern int deviceCount;

void createDevice(char *, void (*)(int, off_t, size_t), struct stat *);
