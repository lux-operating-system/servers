/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * pci: Driver and enumerator for PCI (Express)
 */

#include <pci/pci.h>
#include <liblux/liblux.h>
#include <stdlib.h>
#include <string.h>

/* pciReadFile(): reads from a PCI configuration space file under /dev
 * params: rcmd - read command message
 * returns: nothing, response relayed to devfs
 */

void pciReadFile(RWCommand *rcmd) {
    
}