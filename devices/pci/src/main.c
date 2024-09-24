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

    pciEnumerate();

    SysInfoResponse res;
    luxSysinfo(&res);

    luxLogf(KPRINT_LEVEL_DEBUG, "MEM USAGE: %d MiB / %d MiB\n", res.memoryUsage*res.pageSize/0x100000, res.memorySize*res.pageSize/0x100000);
    luxLogf(KPRINT_LEVEL_DEBUG, "MEM USAGE: %d pages / %d\n", res.memoryUsage, res.memorySize);
    luxLogf(KPRINT_LEVEL_DEBUG, "PROCESSES: %d, THREADS: %d\n", res.processes, res.threads);

    while(1);
}