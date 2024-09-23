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

/* ptyRead(): reads from a pty device
 * params: rcmd - read command message
 * returns: nothing
 */

void ptyRead(RWCommand *rcmd) {
    rcmd->header.header.response = 1;
    rcmd->header.header.length = sizeof(RWCommand);

    // determine if we're reading from the master or the slave
    if(!strcmp(rcmd->path, "/ptmx")) {
        // master, so we read from the slave
        int id = rcmd->id;
        if(!ptys[id].slave || !ptys[id].slaveDataSize || !ptys[id].slaveSize) {
            rcmd->header.header.status = -EWOULDBLOCK;  // no data available
            rcmd->length = 0;
            luxSendDependency(rcmd);
            return;
        }

        size_t truelen;
        if(rcmd->length > ptys[id].slaveDataSize) truelen = ptys[id].slaveDataSize;
        else truelen = rcmd->length;

        // and read the data
        memcpy(rcmd->data, ptys[id].slave, truelen);
        memmove(ptys[id].slave, (const void *)(uintptr_t)ptys[id].slave + truelen, ptys[id].slaveDataSize - truelen);
        ptys[id].slaveDataSize -= truelen;

        // respond
        rcmd->header.header.status = truelen;
        rcmd->header.header.length += truelen;
        rcmd->length = truelen;
        luxSendDependency(rcmd);
    } else {
        // slave, read from the master
        int id = atoi(&rcmd->path[4]);
        if(!ptys[id].master || !ptys[id].masterDataSize || !ptys[id].masterSize) {
            rcmd->header.header.status = -EWOULDBLOCK;  // no data available
            rcmd->length = 0;
            luxSendDependency(rcmd);
            return;
        }

        size_t truelen;
        if(rcmd->length > ptys[id].masterDataSize) truelen = ptys[id].masterDataSize;
        else truelen = rcmd->length;

        // and read the data
        memcpy(rcmd->data, ptys[id].master, truelen);
        memmove(ptys[id].master, (const void *)(uintptr_t)ptys[id].master + truelen, ptys[id].masterDataSize - truelen);
        ptys[id].masterDataSize -= truelen;

        // respond
        rcmd->header.header.status = truelen;
        rcmd->header.header.length += truelen;
        rcmd->length = truelen;
        luxSendDependency(rcmd);
    }
}