/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * ide: Device driver for IDE (ATA HDDs)
 */

#include <liblux/liblux.h>
#include <liblux/sdev.h>
#include <ide/ide.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>

int main(void) {
    luxInit("ide");
    while(luxConnectDependency("sdev"));

    // scan the PCI bus
    DIR *dir = opendir("/dev/pci");
    struct dirent *entry;
    seekdir(dir, 2);    // skip "." and ".."

    char path[32];
    uint8_t class[3];

    while((entry = readdir(dir))) {
        sprintf(path, "/dev/pci/%s/class", entry->d_name);

        FILE *file = fopen(path, "r");
        if(!file) continue;

        if(fread(&class, 1, 3, file) != 3) {
            fclose(file);
            continue;
        }

        fclose(file);

        if(class[0] == 0x01 && class[1] == 0x01) {
            luxLogf(KPRINT_LEVEL_DEBUG, "IDE controller at /dev/pci/%s:\n", entry->d_name);
            ideInit(entry->d_name, class[2]);
        }
    }

    //luxReady();

    for(;;) {
        sched_yield();
    }
}