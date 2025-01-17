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
#include <signal.h>
#include <errno.h>

/* pushSec(): helper function to push one character to the secondary terminal
 * params: pty - pty to push to
 * params: c - character to push
 * returns: nothing
 */

static void pushSec(Pty *pty, int c) {
    if(!pty->secondary || !pty->secondarySize) {
        pty->secondary = malloc(32);
        if(!pty->secondary) return;
        pty->secondarySize = 32;
    }

    if(pty->secondaryDataSize+1 > pty->secondarySize) {
        void *newptr = realloc(pty->secondary, pty->secondarySize+32);
        if(!newptr) return;

        pty->secondary = newptr;
        pty->secondarySize += 32;
    }

    char *ptr = pty->secondary;
    *ptr = c;
    pty->secondaryDataSize++;
}

/* ptyWrite(): writes to a pty device
 * params: wcmd - write command message
 * returns: nothing
 */

void ptyWrite(RWCommand *wcmd) {
    wcmd->header.header.response = 1;
    wcmd->header.header.length = sizeof(RWCommand);

    // first determine if we are writing to the primary or the secondary
    if(!strcmp(wcmd->path, "/ptmx")) {
        // primary
        int id = wcmd->id;
        if(!ptys[id].primary) {
            ptys[id].primary = malloc(wcmd->length);
            if(!ptys[id].primary) {
                wcmd->header.header.status = -ENOMEM;
                wcmd->length = 0;
                luxSendKernel(wcmd);
                return;
            }

            ptys[id].primarySize = wcmd->length;
        }

        if((wcmd->length + ptys[id].primaryDataSize) > ptys[id].primarySize) {
            // allocate more memory if necessary
            void *newptr = realloc(ptys[id].primary, wcmd->length + ptys[id].primaryDataSize);
            if(!newptr) {
                wcmd->header.header.status = -ENOMEM;
                wcmd->length = 0;
                luxSendKernel(wcmd);
                return;
            }

            ptys[id].primary = newptr;
            ptys[id].primarySize += wcmd->length;
        }

        // check for control characters
        if(ptys[id].termios.c_lflag & ISIG) {
            char control = wcmd->data[0];
            if(control == ptys[id].termios.c_cc[VINTR]) {
                kill(-1 * ptys[id].group, SIGINT);
                wcmd->header.header.status = wcmd->length;
                if(!wcmd->silent) luxSendKernel(wcmd);
                return;
            } else if(control == ptys[id].termios.c_cc[VQUIT]) {
                kill(-1 * ptys[id].group, SIGQUIT);
                wcmd->header.header.status = wcmd->length;
                if(!wcmd->silent) luxSendKernel(wcmd);
                return;
            }
        }

        // now we can safely write to the buffer
        if(!(ptys[id].termios.c_lflag & ICANON)) {
            memcpy((void *)((uintptr_t)ptys[id].primary + ptys[id].primaryDataSize), wcmd->data, wcmd->length);
            ptys[id].primaryDataSize += wcmd->length;
        } else {
            // for canonical mode, we need special handling for backspace
            char *input = (char *) wcmd->data;
            char *primary = (char *) ptys[id].primary;
            for(int i = 0; i < wcmd->length; i++) {
                if(input[i] == '\b') {
                    if(ptys[id].primaryDataSize) {
                        ptys[id].primaryDataSize--;
                        if(ptys[id].termios.c_lflag & ECHO) pushSec(&ptys[id], '\b');
                    }
                } else {
                    primary[ptys[id].primaryDataSize] = input[i];
                    ptys[id].primaryDataSize++;
                    if(ptys[id].termios.c_lflag & ECHO) pushSec(&ptys[id], input[i]);
                }
            }
        }

        // if ECHO is enabled, write to the secondary as well for echo
        if((!(ptys[id].termios.c_lflag & ICANON) && (ptys[id].termios.c_lflag & ECHO))) {
            if(!ptys[id].secondary) {
                ptys[id].secondary = malloc(wcmd->length);
                if(!ptys[id].secondary) {
                    wcmd->header.header.status = -ENOMEM;
                    wcmd->length = 0;
                    luxSendKernel(wcmd);
                    return;
                }

                ptys[id].secondarySize = wcmd->length;
            }

            if((wcmd->length + ptys[id].secondaryDataSize) > ptys[id].secondarySize) {
                // allocate more memory if necessary
                void *newptr = realloc(ptys[id].secondary, wcmd->length + ptys[id].secondaryDataSize);
                if(!newptr) {
                    wcmd->header.header.status = -ENOMEM;
                    wcmd->length = 0;
                    luxSendKernel(wcmd);
                    return;
                }

                ptys[id].secondary = newptr;
                ptys[id].secondarySize += wcmd->length;
            }

            memcpy((void *)((uintptr_t)ptys[id].secondary + ptys[id].secondaryDataSize), wcmd->data, wcmd->length);
            ptys[id].secondaryDataSize += wcmd->length;
        }

        wcmd->header.header.status = wcmd->length;
        if(!wcmd->silent) luxSendKernel(wcmd);
    } else {
        // secondary
        int id = atoi(&wcmd->path[4]);
        if(!ptys[id].secondary) {
            ptys[id].secondary = malloc(wcmd->length);
            if(!ptys[id].secondary) {
                wcmd->header.header.status = -ENOMEM;
                wcmd->length = 0;
                luxSendKernel(wcmd);
                return;
            }

            ptys[id].secondarySize = wcmd->length;
        }

        if((wcmd->length + ptys[id].secondaryDataSize) > ptys[id].secondarySize) {
            // allocate more memory if necessary
            void *newptr = realloc(ptys[id].secondary, wcmd->length + ptys[id].secondaryDataSize);
            if(!newptr) {
                wcmd->header.header.status = -ENOMEM;
                wcmd->length = 0;
                luxSendKernel(wcmd);
                return;
            }

            ptys[id].secondary = newptr;
            ptys[id].secondarySize += wcmd->length;
        }

        // now we can safely write to the buffer
        memcpy((void *)((uintptr_t)ptys[id].secondary + ptys[id].secondaryDataSize), wcmd->data, wcmd->length);
        ptys[id].secondaryDataSize += wcmd->length;

        wcmd->header.header.status = wcmd->length;
        if(!wcmd->silent) luxSendKernel(wcmd);
    }
}

/* ptyRead(): reads from a pty device
 * params: rcmd - read command message
 * returns: nothing
 */

void ptyRead(RWCommand *rcmd) {
    rcmd->header.header.response = 1;
    rcmd->header.header.length = sizeof(RWCommand);

    // determine if we're reading from the primary or the secondary
    if(!strcmp(rcmd->path, "/ptmx")) {
        // primary, so we read from the secondary
        int id = rcmd->id;
        if(!ptys[id].secondary || !ptys[id].secondaryDataSize || !ptys[id].secondarySize) {
            rcmd->header.header.status = -EWOULDBLOCK;  // no data available
            rcmd->length = 0;
            luxSendKernel(rcmd);
            return;
        }

        size_t truelen;
        if(rcmd->length > ptys[id].secondaryDataSize) truelen = ptys[id].secondaryDataSize;
        else truelen = rcmd->length;

        // and read the data
        memcpy(rcmd->data, ptys[id].secondary, truelen);
        memmove(ptys[id].secondary, (const void *)(uintptr_t)ptys[id].secondary + truelen, ptys[id].secondaryDataSize - truelen);
        ptys[id].secondaryDataSize -= truelen;

        // respond
        rcmd->header.header.status = truelen;
        rcmd->header.header.length += truelen;
        rcmd->length = truelen;
        luxSendKernel(rcmd);
    } else {
        // secondary, read from the primary
        int id = atoi(&rcmd->path[4]);
        if(!ptys[id].primary || !ptys[id].primaryDataSize || !ptys[id].primarySize) {
            rcmd->header.header.status = -EWOULDBLOCK;  // no data available
            rcmd->length = 0;
            luxSendKernel(rcmd);
            return;
        }

        size_t truelen;
        if(rcmd->length > ptys[id].primaryDataSize) truelen = ptys[id].primaryDataSize;
        else truelen = rcmd->length;

        if(!(ptys[id].termios.c_lflag & ICANON)) {
            memcpy(rcmd->data, ptys[id].primary, truelen);
            memmove(ptys[id].primary, (const void *)(uintptr_t)ptys[id].primary + truelen, ptys[id].primaryDataSize - truelen);
            ptys[id].primaryDataSize -= truelen;

            rcmd->header.header.status = truelen;
            rcmd->header.header.length += truelen;
            rcmd->length = truelen;
        } else {
            // in canonical mode, no input is available until the user presses enter
            const char *primary = (const char *) ptys[id].primary;

            bool readable = false;
            for(int i = 0; i < ptys[id].primaryDataSize; i++) {
                if(primary[i] == '\n') {
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

                if(primary[i] == '\b') {
                    if(rcmd->length) {
                        rcmd->length--;
                        rcmd->header.header.status--;
                        rcmd->header.header.length--;
                    }
                } else {
                    data[rcmd->length] = primary[i];
                    rcmd->length++;
                    rcmd->header.header.status++;
                    rcmd->header.header.length++;
                    if(primary[i] == '\n') break;
                }
            }

            memmove(ptys[id].primary, (const void *)(uintptr_t)ptys[id].primary + canonicalCount, ptys[id].primaryDataSize - canonicalCount);
            ptys[id].primaryDataSize -= canonicalCount;
        }

        luxSendKernel(rcmd);
    }
}

/* ptyFsync(): implementation of fsync() for pseudo-terminal devices
 * params: cmd - fsync command message
 * returns: nothing, response relayed to kernel
 */

void ptyFsync(FsyncCommand *cmd) {
    /* there really isn't anything to do here because we don't support real
     * hardware terminals, but the kernel doesn't know that, so just return
     * a success return value */
    cmd->header.header.response = 1;
    cmd->header.header.status = 0;
    luxSendKernel(cmd);
}