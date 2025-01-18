/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024-25
 * 
 * ide: Device driver for IDE (ATA HDDs)
 */

#include <liblux/liblux.h>
#include <liblux/sdev.h>
#include <ide/ide.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdio.h>
#include <errno.h>

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

    MessageHeader *msg = calloc(1, SERVER_MAX_SIZE);
    if(!msg) {
        luxLogf(KPRINT_LEVEL_ERROR, "failed to allocate memory for message passing\n");
        for(;;);
    }

    luxReady();

    for(;;) {
        int busy = 0;
        ssize_t s = luxRecvDependency(msg, SERVER_MAX_SIZE, false, true);
        if(s > 0 && s <= SERVER_MAX_SIZE) {
            busy++;
            if(msg->length > SERVER_MAX_SIZE) {
                void *newptr = realloc(msg, msg->length);
                if(!newptr) {
                    luxLogf(KPRINT_LEVEL_ERROR, "unable to allocate memory for I/O\n");
                    msg->length = sizeof(MessageHeader);
                    msg->status = -ENOMEM;
                    msg->response = 1;
                    luxSendDependency(msg);
                    continue;
                }

                msg = newptr;
            }

            luxRecvDependency(msg, msg->length, false, false);

            switch(msg->command) {
            case COMMAND_SDEV_READ: ideRead((SDevRWCommand *) msg); break;
            default:
                luxLogf(KPRINT_LEVEL_WARNING, "unimplemented command 0x%04X\n", msg->command);
                msg->length = sizeof(MessageHeader);
                msg->status = -ENOSYS;
                msg->response = 1;
                luxSendDependency(msg);
            }
        }

        if(!busy) sched_yield();
    }
}