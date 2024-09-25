/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * pci: Driver and enumerator for PCI (Express)
 */

#include <pci/pci.h>
#include <liblux/liblux.h>
#include <sys/io.h>
#include <stdlib.h>

int main() {
    luxInit("pci");
    while(luxConnectDependency("devfs"));   // depend on /dev

    // command buffer
    SyscallHeader *cmd = calloc(1, SERVER_MAX_SIZE);
    if(!cmd) {
        luxLogf(KPRINT_LEVEL_ERROR, "unable to allocate memory for PCI server\n");
        return -1;
    }

    // request I/O port access
    if(ioperm(PCI_CONFIG_ADDRESS, 8, 1)) {
        luxLogf(KPRINT_LEVEL_ERROR, "unable to get access to I/O ports\n");
        return -1;
    }

    pciEnumerate();

    // notify lumen that the server is ready
    luxReady();

    while(1) {
        ssize_t s = luxRecvDependency(cmd, SERVER_MAX_SIZE, false, false);
        if(s <= 0) continue;

        switch(cmd->header.command) {
        case COMMAND_READ: pciReadFile((RWCommand *) cmd); break;
        default:
            luxLogf(KPRINT_LEVEL_WARNING, "unimplemented command 0x%04X, dropping message...\n", cmd->header.command);
        }
    }
}