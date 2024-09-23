/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * pty: Microkernel server implementing Unix 98-style pseudo-terminal devices
 */

#include <liblux/liblux.h>
#include <pty/pty.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

/* ptyWrite(): writes to a pty device
 * params: wcmd - write command message
 * returns: nothing
 */

void ptyWrite(RWCommand *wcmd) {
    wcmd->header.header.response = 1;
    wcmd->header.header.length = sizeof(RWCommand);

    // first determine if we are writing to the master or the slave
    if(!strcmp(wcmd->path, "/ptmx")) {
        // master
        int id = wcmd->id;
        if(!ptys[id].master) {
            ptys[id].master = malloc(wcmd->length);
            if(!ptys[id].master) {
                wcmd->header.header.status = -ENOMEM;
                wcmd->length = 0;
                luxSendDependency(wcmd);
                return;
            }

            ptys[id].masterSize = wcmd->length;
        }

        if((wcmd->length + ptys[id].masterDataSize) > ptys[id].masterSize) {
            // allocate more memory if necessary
            void *newptr = realloc(ptys[id].master, wcmd->length + ptys[id].masterDataSize);
            if(!newptr) {
                wcmd->header.header.status = -ENOMEM;
                wcmd->length = 0;
                luxSendDependency(wcmd);
                return;
            }

            ptys[id].master = newptr;
        }

        // now we can safely write to the buffer
        memcpy((void *)((uintptr_t)ptys[id].master + ptys[id].masterDataSize), wcmd->data, wcmd->length);
        ptys[id].masterDataSize += wcmd->length;

        wcmd->header.header.status = wcmd->length;
        luxSendDependency(wcmd);
    } else {
        // slave
        int id = atoi(&wcmd->path[4]);
        if(!ptys[id].slave) {
            ptys[id].slave = malloc(wcmd->length);
            if(!ptys[id].slave) {
                wcmd->header.header.status = -ENOMEM;
                wcmd->length = 0;
                luxSendDependency(wcmd);
                return;
            }

            ptys[id].slaveSize = wcmd->length;
        }

        if((wcmd->length + ptys[id].slaveDataSize) > ptys[id].slaveSize) {
            // allocate more memory if necessary
            void *newptr = realloc(ptys[id].slave, wcmd->length + ptys[id].slaveDataSize);
            if(!newptr) {
                wcmd->header.header.status = -ENOMEM;
                wcmd->length = 0;
                luxSendDependency(wcmd);
                return;
            }

            ptys[id].slave = newptr;
        }

        // now we can safely write to the buffer
        memcpy((void *)((uintptr_t)ptys[id].slave + ptys[id].slaveDataSize), wcmd->data, wcmd->length);
        ptys[id].slaveDataSize += wcmd->length;

        wcmd->header.header.status = wcmd->length;
        luxSendDependency(wcmd);
    }
}