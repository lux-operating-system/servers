/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * pci: Driver and enumerator for PCI (Express)
 */

#include <pci/pci.h>
#include <liblux/liblux.h>
#include <liblux/devfs.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

static PCIFile *files;

/* pciCreateFile(): creates a file under /dev for a PCI device
 * params: bus - PCI bus
 * params: slot - PCI slot
 * params: function - PCI function
 * params: reg - corresponding PCI configuration register
 * params: write - write access
 * params: path - file name
 * params: size - file size
 * params: data - data buffer of specified size
 * returns: nothing
 */

void pciCreateFile(uint8_t bus, uint8_t slot, uint8_t function, uint16_t reg,
                   int write, const char *path, size_t size, void *data) {
    DevfsRegisterCommand regcmd;
    memset(&regcmd, 0, sizeof(DevfsRegisterCommand));

    regcmd.header.command = COMMAND_DEVFS_REGISTER;
    regcmd.header.length = sizeof(DevfsRegisterCommand);
    strcpy(regcmd.server, "lux:///dspci");
    sprintf(regcmd.path, "/pci/%02x.%02x.%02x/%s", bus, slot, function, path);

    // character device owned by root:root, r--r--r--
    regcmd.status.st_mode = S_IRUSR | S_IRGRP | S_IROTH | S_IFCHR;
    if(write) regcmd.status.st_mode |= S_IWUSR;
    regcmd.status.st_size = size;

    luxSendDependency(&regcmd);
    for(int i = 0; i < 4; i++) sched_yield();
}