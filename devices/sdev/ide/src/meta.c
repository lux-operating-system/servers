/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024-25
 * 
 * ide: Device driver for IDE (ATA HDDs)
 */

#include <liblux/liblux.h>
#include <liblux/sdev.h>
#include <ide/ide.h>
#include <sys/io.h>
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

    if(progif & 0x04) {
        sprintf(path, "/dev/pci/%s/bar2", address);
        FILE *file = fopen(path, "rb");
        if(!file) goto fail;

        size_t s = fread(&ide->secondaryBase, 1, 2, file);
        fclose(file);
        if(s != 2) goto fail;

        sprintf(path, "/dev/pci/%s/bar3", address);
        file = fopen(path, "rb");
        if(!file) goto fail;

        s = fread(&ide->secondaryStatus, 1, 2, file);
        fclose(file);
        if(s != 2) goto fail;
        ide->secondaryStatus += 2;
    } else {
        ide->secondaryBase = ATA_SECONDARY_BASE;
        ide->secondaryStatus = ATA_SECONDARY_STATUS;
    }

    if(ioperm(ide->secondaryBase, 8, 1)) {
        luxLogf(KPRINT_LEVEL_ERROR, "failed to acquire I/O port 0x%04X\n", ide->secondaryBase);
        goto fail;
    }

    if(ioperm(ide->secondaryStatus, 1, 1)) {
        luxLogf(KPRINT_LEVEL_ERROR, "failed to acquire I/O port 0x%04X\n", ide->secondaryStatus);
        goto fail;
    }

    if(ioperm(ide->primaryBase, 8, 1)) {
        luxLogf(KPRINT_LEVEL_ERROR, "failed to acquire I/O port 0x%04X\n", ide->primaryBase);
        goto fail;
    }

    if(ioperm(ide->primaryStatus, 1, 1)) {
        luxLogf(KPRINT_LEVEL_ERROR, "failed to acquire I/O port 0x%04X\n", ide->primaryStatus);
        goto fail;
    }

    luxLogf(KPRINT_LEVEL_DEBUG, "- primary: %s %s mode: I/O ports 0x%04X, 0x%04X\n",
        progif & 0x02 ? "variable" : "fixed",
        progif & 0x01 ? "native" : "compatibility",
        ide->primaryBase, ide->primaryStatus);

    int drives = 0;
    if(!ataIdentify(ide, 0, 0)) drives++;
    if(!ataIdentify(ide, 0, 1)) drives++;

    luxLogf(KPRINT_LEVEL_DEBUG, "- secondary: %s %s mode: I/O ports 0x%04X, 0x%04X\n",
        progif & 0x02 ? "variable" : "fixed",
        progif & 0x01 ? "native" : "compatibility",
        ide->secondaryBase, ide->secondaryStatus);

    if(!ataIdentify(ide, 1, 0)) drives++;
    if(!ataIdentify(ide, 1, 1)) drives++;

    if(drives) ideRegister(ide);
    else goto fail;

    return;

fail:
    free(ide);
}

/* ideGetDrive(): returns a pointer to an ATA device from its identifier
 * params: id - internal drive identifier:
 *  bit 0 - port 0/1 toggle
 *  bit 1 - primary/secondary channel toggle
 *  higher bits - IDE controller index
 * returns: pointer to ATA device, NULL if no such device
 */

ATADevice *ideGetDrive(uint64_t id) {
    int ctrlIndex = id >> 2;
    if(ctrlIndex >= controllerCount) return NULL;

    IDEController *ctrl = controllers;
    for(int i = 0; ctrlIndex && i < ctrlIndex; i++) {
        ctrl = ctrl->next;
        if(!ctrl) return NULL;
    }

    ATADevice *dev;
    if(id & 2) dev = &ctrl->secondary[id&1];
    else dev = &ctrl->primary[id&1];

    if(dev->valid) return dev;
    return NULL;
}

/* ideRegister(): registers an IDE controller
 * params: ctrl - pointer to controller structure
 * returns: nothing
 */

void ideRegister(IDEController *ctrl) {
    if(!controllers) {
        controllers = ctrl;
        controllerCount++;
        return;
    }

    IDEController *list = controllers;
    while(list->next) list = list->next;
    list->next = ctrl;
    controllerCount++;
}