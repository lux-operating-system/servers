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

/* pushSlave(): helper function to push one character to the slave terminal
 * params: pty - pty to push to
 * params: c - character to push
 * returns: nothing
 */

static void pushSlave(Pty *pty, int c) {
    if(!pty->slave || !pty->slaveSize) {
        pty->slave = malloc(32);
        if(!pty->slave) return;
        pty->slaveSize = 32;
    }

    if(pty->slaveDataSize+1 > pty->slaveSize) {
        void *newptr = realloc(pty->slave, pty->slaveSize+32);
        if(!newptr) return;

        pty->slave = newptr;
        pty->slaveSize += 32;
    }

    char *ptr = pty->slave;
    *ptr = c;
    pty->slaveDataSize++;
}

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
                luxSendKernel(wcmd);
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
                luxSendKernel(wcmd);
                return;
            }

            ptys[id].master = newptr;
            ptys[id].masterSize += wcmd->length;
        }

        // now we can safely write to the buffer
        if(!(ptys[id].termios.c_lflag & ICANON)) {
            memcpy((void *)((uintptr_t)ptys[id].master + ptys[id].masterDataSize), wcmd->data, wcmd->length);
            ptys[id].masterDataSize += wcmd->length;
        } else {
            // for canonical mode, we need special handling for backspace
            char *input = (char *) wcmd->data;
            char *master = (char *) ptys[id].master;
            for(int i = 0; i < wcmd->length; i++) {
                if(input[i] == '\b') {
                    if(ptys[id].masterDataSize) {
                        ptys[id].masterDataSize--;
                        if(ptys[id].termios.c_lflag & ECHO) pushSlave(&ptys[id], '\b');
                    }
                } else {
                    master[ptys[id].masterDataSize] = input[i];
                    ptys[id].masterDataSize++;
                    if(ptys[id].termios.c_lflag & ECHO) pushSlave(&ptys[id], input[i]);
                }
            }
        }

        // if ECHO is enabled, write to the slave as well for echo
        if((!(ptys[id].termios.c_lflag & ICANON) && (ptys[id].termios.c_lflag & ECHO))) {
            if(!ptys[id].slave) {
                ptys[id].slave = malloc(wcmd->length);
                if(!ptys[id].slave) {
                    wcmd->header.header.status = -ENOMEM;
                    wcmd->length = 0;
                    luxSendKernel(wcmd);
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
                    luxSendKernel(wcmd);
                    return;
                }

                ptys[id].slave = newptr;
                ptys[id].slaveSize += wcmd->length;
            }

            memcpy((void *)((uintptr_t)ptys[id].slave + ptys[id].slaveDataSize), wcmd->data, wcmd->length);
            ptys[id].slaveDataSize += wcmd->length;
        }

        wcmd->header.header.status = wcmd->length;
        luxSendKernel(wcmd);
    } else {
        // slave
        int id = atoi(&wcmd->path[4]);
        if(!ptys[id].slave) {
            ptys[id].slave = malloc(wcmd->length);
            if(!ptys[id].slave) {
                wcmd->header.header.status = -ENOMEM;
                wcmd->length = 0;
                luxSendKernel(wcmd);
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
                luxSendKernel(wcmd);
                return;
            }

            ptys[id].slave = newptr;
            ptys[id].slaveSize += wcmd->length;
        }

        // now we can safely write to the buffer
        memcpy((void *)((uintptr_t)ptys[id].slave + ptys[id].slaveDataSize), wcmd->data, wcmd->length);
        ptys[id].slaveDataSize += wcmd->length;

        wcmd->header.header.status = wcmd->length;
        luxSendKernel(wcmd);
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
            luxSendKernel(rcmd);
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
        luxSendKernel(rcmd);
    } else {
        // slave, read from the master
        int id = atoi(&rcmd->path[4]);
        if(!ptys[id].master || !ptys[id].masterDataSize || !ptys[id].masterSize) {
            rcmd->header.header.status = -EWOULDBLOCK;  // no data available
            rcmd->length = 0;
            luxSendKernel(rcmd);
            return;
        }

        size_t truelen;
        if(rcmd->length > ptys[id].masterDataSize) truelen = ptys[id].masterDataSize;
        else truelen = rcmd->length;

        if(!(ptys[id].termios.c_lflag & ICANON)) {
            memcpy(rcmd->data, ptys[id].master, truelen);
            memmove(ptys[id].master, (const void *)(uintptr_t)ptys[id].master + truelen, ptys[id].masterDataSize - truelen);
            ptys[id].masterDataSize -= truelen;

            rcmd->header.header.status = truelen;
            rcmd->header.header.length += truelen;
            rcmd->length = truelen;
        } else {
            // in canonical mode, no input is available until the user presses enter
            const char *master = (const char *) ptys[id].master;

            bool readable = false;
            for(int i = 0; i < ptys[id].masterDataSize; i++) {
                if(master[i] == '\n') {
                    readable = true;
                    break;
                }
            }

            if(!readable) {
                rcmd->header.header.status = -EWOULDBLOCK;
                rcmd->length = 0;
                luxSendKernel(rcmd);
                return;
            }

            char *data = (char *) rcmd->data;

            rcmd->length = 0;
            rcmd->header.header.status = 0;
            size_t canonicalCount = 0;
            for(int i = 0; i < truelen; i++) {
                canonicalCount++;

                if(master[i] == '\b') {
                    if(rcmd->length) {
                        rcmd->length--;
                        rcmd->header.header.status--;
                        rcmd->header.header.length--;
                    }
                } else {
                    data[rcmd->length] = master[i];
                    rcmd->length++;
                    rcmd->header.header.status++;
                    rcmd->header.header.length++;
                    if(master[i] == '\n') break;
                }
            }

            memmove(ptys[id].master, (const void *)(uintptr_t)ptys[id].master + canonicalCount, ptys[id].masterDataSize - canonicalCount);
            ptys[id].masterDataSize -= canonicalCount;
        }

        luxSendKernel(rcmd);
    }
}