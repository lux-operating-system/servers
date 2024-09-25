/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * pci: Driver and enumerator for PCI (Express)
 */

#include <liblux/liblux.h>
#include <liblux/devfs.h>
#include <string.h>
#include <stdio.h>

/* pciCreateFile(): creates a file under /dev for a PCI device
 * params: bus - PCI bus
 * params: slot - PCI slot
 * params: function - PCI function
 * params: path - file name
 * params: size - file size
 * returns: nothing
 */

void pciCreateFile(uint8_t bus, uint8_t slot, uint8_t function, const char *path, size_t size) {
    DevfsRegisterCommand regcmd;
    memset(&regcmd, 0, sizeof(DevfsRegisterCommand));

    regcmd.header.command = COMMAND_DEVFS_REGISTER;
    regcmd.header.length = sizeof(DevfsRegisterCommand);
    strcpy(regcmd.server, "lux:///dspci");
    sprintf(regcmd.path, "/pci/%02x.%02x.%02x/%s", bus, slot, function, path);

    // character device owned by root:root, rw-r--r--
    regcmd.status.st_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH | S_IFCHR;
    regcmd.status.st_size = size;

    luxSendDependency(&regcmd);
}