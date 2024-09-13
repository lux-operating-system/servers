/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * vfs: Message structures for file system servers that depend on the VFS
 */

#pragma once

#include <liblux/liblux.h>

#define MAX_FILE_SYSTEMS            32

#define COMMAND_VFS_INIT            0xFFFF

typedef struct {
    MessageHeader header;
    char fsType[16];
} VFSInitCommand;

typedef struct {
    int socket;
    char type[16];
} FileSystemServers;
