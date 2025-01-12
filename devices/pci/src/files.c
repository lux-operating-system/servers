/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * pci: Driver and enumerator for PCI (Express)
 */

#include <pci/pci.h>
#include <liblux/liblux.h>
#include <liblux/devfs.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static PCIFile *files = NULL;

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
    PCIFile *file = calloc(1, sizeof(PCIFile));
    if(!file) {
        luxLogf(KPRINT_LEVEL_ERROR, "unable to allocate memory for PCI device\n");
        return;
    }

    if(!files) files = file;
    else {
        PCIFile *list = files;
        while(list->next) list = list->next;

        list->next = file;
    }

    file->bus = bus;
    file->slot = slot;
    file->function = function;
    file->reg = reg;
    file->size = size;
    memcpy(file->data, data, size);

    DevfsRegisterCommand regcmd;
    memset(&regcmd, 0, sizeof(DevfsRegisterCommand));

    regcmd.header.command = COMMAND_DEVFS_REGISTER;
    regcmd.header.length = sizeof(DevfsRegisterCommand);
    strcpy(regcmd.server, "lux:///dspci");
    sprintf(regcmd.path, "/pci/%02x.%02x.%02x/%s", bus, slot, function, path);

    strcpy(file->name, regcmd.path);

    // character device owned by root:root, r--r--r--
    regcmd.status.st_mode = S_IRUSR | S_IRGRP | S_IROTH | S_IFCHR;
    if(write) regcmd.status.st_mode |= S_IWUSR;
    regcmd.status.st_size = size;

    luxSendDependency(&regcmd);

    ssize_t rs = luxRecvDependency(&regcmd, regcmd.header.length, true, false);
    if(rs < sizeof(DevfsRegisterCommand) || regcmd.header.status
    || regcmd.header.command != COMMAND_DEVFS_REGISTER) {
        luxLogf(KPRINT_LEVEL_ERROR, "failed to register /dev/%s, error code = %d\n", regcmd.path, regcmd.header.status);
    }
}

/* pciFindFile(): finds a PCI file structure by name
 * params: path - file name
 * returns: pointer to PCI file structure, NULL on error
 */

PCIFile *pciFindFile(const char *path) {
    if(!files) return NULL;

    PCIFile *list = files;
    while(list) {
        if(!strcmp(list->name, path)) return list;
        else list = list->next;
    }

    return NULL;
}