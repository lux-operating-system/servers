/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * pci: Driver and enumerator for PCI (Express)
 */

#include <liblux/liblux.h>

int main() {
    luxInit("pci");
    while(luxConnectDependency("devfs"));   // depend on /dev

    while(1);
}