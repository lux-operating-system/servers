/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * ide: Device driver for IDE (ATA HDDs)
 */

#include <liblux/liblux.h>
#include <liblux/sdev.h>
#include <ide/ide.h>
#include <stdlib.h>
#include <stdio.h>

IDEController *controllers = NULL;
int controllerCount = 0;

/* ideInit(): initializes and detects drives on a PCI IDE controller
 * params: address - PCI address of the controller
 * params: progif - programming interface byte from the PCI configuration
 * returns: nothing
 */

void ideInit(const char *address, uint8_t progif) {
    IDEController *ide = calloc(1, sizeof(IDEController));
    if(!ide) {
        luxLogf(KPRINT_LEVEL_ERROR, "failed to allocate memory for IDE controller\n");
        return;
    }

    char path[32];

    // detect the controller's I/O ports
    if(progif & 0x01) {
        sprintf(path, "/dev/pci/%s/bar0", address);
        FILE *file = fopen(path, "rb");
        if(!file) goto fail;

        size_t s = fread(&ide->primaryBase, 1, 2, file);
        fclose(file);
        if(s != 2) goto fail;

        sprintf(path, "/dev/pci/%s/bar1", address);
        file = fopen(path, "rb");
        if(!file) goto fail;

        s = fread(&ide->primaryStatus, 1, 2, file);
        fclose(file);
        if(s != 2) goto fail;
        ide->primaryStatus += 2;
    } else {
        ide->primaryBase = ATA_PRIMARY_BASE;
        ide->primaryStatus = ATA_PRIMARY_STATUS;
    }

    luxLogf(KPRINT_LEVEL_DEBUG, "- primary: %s %s mode: I/O ports 0x%04X, 0x%04X\n",
        progif & 0x02 ? "variable" : "fixed",
        progif & 0x01 ? "native" : "compatibility",
        ide->primaryBase, ide->primaryStatus);

    return;

fail:
    free(ide);
}