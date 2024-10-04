/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * vfs: Microkernel server implementing a virtual file system
 */

#pragma once

#include <liblux/liblux.h>
#include <vfs.h>

#define MAX_MOUNTPOINTS             128

extern void (*vfsDispatchTable[])(SyscallHeader *);
extern FileSystemServers *servers;
extern int serverCount;

typedef struct {
    char *device;
    char *path;
    char type[16];
    int flags;
    int valid;
} Mountpoint;

extern Mountpoint *mps;
extern int mpCount;

int findFSServer(const char *);
void registerMountpoint(MountCommand *);
char *resolve(char *, char *, char *, char *);