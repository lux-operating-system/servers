/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * sdev: Abstraction for storage devices under /dev/sdX
 */

#include <liblux/liblux.h>
#include <liblux/sdev.h>
#include <sdev/sdev.h>
#include <stdlib.h>
#include <unistd.h>         // sched_yield()

#define MAX_DRIVERS         8

static int *connections;

int main() {
    luxInit("sdev");
    while(luxConnectDependency("devfs"));   // obviously depend on devfs

    connections = calloc(MAX_DRIVERS, sizeof(int));
    MessageHeader *msg = calloc(1, SERVER_MAX_SIZE);

    if(!connections || !msg) {
        luxLogf(KPRINT_LEVEL_ERROR, "unable to allocate memory for storage device layer\n");
        return -1;
    }

    // there really isn't anything to do here until storage device drivers are
    // loaded, so notify lumen that we're ready
    luxReady();

    for(;;) {
        int actions = 0;

        // wait for connections from device drivers
        if(drvCount < MAX_DRIVERS) {
            int sd = luxAccept();
            if(sd > 0) {
                actions++;
                connections[drvCount] = sd;
                drvCount++;
            }
        }

        // receive requests and responses from device drivers
        for(int i = 0; drvCount && (i < drvCount); i++) {
            ssize_t s = luxRecv(connections[i], msg, SERVER_MAX_SIZE, false, true);     // peek first
            if(s > 0 && s <= SERVER_MAX_SIZE) {
                actions++;
                if(msg->length > SERVER_MAX_SIZE) {
                    void *newptr = realloc(msg, msg->length);
                    if(!newptr) {
                        luxLogf(KPRINT_LEVEL_ERROR, "unable to allocate memory to handle I/O\n");
                        return -1;
                    }

                    msg = newptr;
                }

                luxRecv(connections[i], msg, msg->length, false, false);    // receive the actual msg
                switch(msg->command) {
                case COMMAND_SDEV_REGISTER: registerDevice(connections[i], (SDevRegisterCommand *) msg); break;
                case COMMAND_SDEV_READ: relayRead((SDevRWCommand *) msg); break;
                default:
                    luxLogf(KPRINT_LEVEL_WARNING, "unimplemented command 0x%04X from storage device driver, dropping message\n", msg->command);
                }
            }
        }

        // and requests from devfs
        ssize_t s = luxRecvDependency(msg, SERVER_MAX_SIZE, false, true);   // peek
        if(s > 0 && s <= SERVER_MAX_SIZE) {
            actions++;
            if(msg->length > SERVER_MAX_SIZE) {
                void *newptr = realloc(msg, msg->length);
                if(!newptr) {
                    luxLogf(KPRINT_LEVEL_ERROR, "unable to allocate memory to handle I/O\n");
                    return -1;
                }

                msg = newptr;
            }

            luxRecvDependency(msg, msg->length, false, false);
            switch(msg->command) {
            case COMMAND_READ: sdevRead((RWCommand *) msg); break;
            default:
                luxLogf(KPRINT_LEVEL_WARNING, "unimplemented command 0x%04X from devfs, dropping message...\n", msg->command);
            }
        }

        if(!actions) sched_yield();
    }
}