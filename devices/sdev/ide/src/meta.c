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
}