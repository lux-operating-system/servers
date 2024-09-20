/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * liblux: Library abstracting kernel-server communication protocols
 */

#pragma once

#include <liblux/liblux.h>
#include <sys/stat.h>

/* These commands are requested by device drivers from devfs */
#define COMMAND_DEVFS_REGISTER          0xD000  /* register a device on /dev */
#define COMMAND_DEVFS_UNREGISTER        0xD001  /* unregister a device */
#define COMMAND_DEVFS_STATUS            0xD002  /* device status */

typedef struct {
    MessageHeader header;
    char path[MAX_FILE_PATH];           // path on /dev
    char server[256];                   // driver to handle the request
    struct stat status;
} DevfsRegisterCommand;
