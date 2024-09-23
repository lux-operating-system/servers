/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * pci: Driver and enumerator for PCI (Express)
 */

#include <pci/pci.h>
#include <liblux/liblux.h>
#include <sys/io.h>

int main() {
    luxInit("pci");
    while(luxConnectDependency("devfs"));   // depend on /dev

    // request I/O port access
    if(ioperm(PCI_CONFIG_ADDRESS, 8, 1)) {
        luxLogf(KPRINT_LEVEL_ERROR, "unable to get access to I/O ports\n");
        return -1;
    }

    while(1);
}