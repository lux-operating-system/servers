/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * nvme: Device driver for NVMe SSDs
 */

/* References:
 * - NVM Express Base Specification 2.1 (5 Aug 2024)
 * - NVM Command Set Specification 1.1 (5 Aug 2024)
 * - NVM Express over PCIe Transport Specification 1.1 (5 Aug 2024)
 */

#include <liblux/liblux.h>
#include <nvme/nvme.h>
#include <dirent.h>
#include <stdio.h>
#include <unistd.h>

int main() {
    luxInit("nvme");

    // TODO: depend on sdev, the generic storage device abstraction layer
    //while(luxConnectDependency("sdev"));

    // enumerate PCI devices under /dev/pci/ and search for NVMe devices
    // NVM Express has class 0x01, subclass 0x08, and interface 0x02
    DIR *dir = opendir("/dev/pci");
    if(!dir) {
        luxLogf(KPRINT_LEVEL_WARNING, "unable to open directory /dev/pci\n");
        return -1;
    }

    char path[32];
    uint8_t class[3];

    struct dirent *entry;
    seekdir(dir, 2);        // skip over '.' and '..'
    while((entry = readdir(dir))) {
        // try to read the class
        sprintf(path, "/dev/pci/%s/class", entry->d_name);
        
        FILE *file = fopen(path, "rb");
        if(!file) continue;

        if(fread(&class, 1, 3, file) != 3) {
            fclose(file);
            continue;
        }

        fclose(file);

        if(class[0] == 0x01 && class[1] == 0x08 && class[2] == 0x02) {
            luxLogf(KPRINT_LEVEL_DEBUG, "NVMe controller at /dev/pci/%s:\n", entry->d_name);
            nvmeInit(entry->d_name);
        }
    }

    closedir(dir);

    for(;;);
}