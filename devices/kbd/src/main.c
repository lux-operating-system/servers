/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * kbd: Abstraction for keyboard devices under /dev/kbd
 */

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <liblux/liblux.h>
#include <liblux/devfs.h>

#define MAX_KEYBOARDS   16  /* arbitrary number? idk i don't imagine this will ever matter */
#define KEYBOARD_BUFFER 32

static int *connections;
static int kbdCount = 0;
static uint16_t *buffer;           // keypress buffer
static int bufferSize = 0;         // number of keypresses in buffer

int main() {
    luxInit("kbd");
    while(luxConnectDependency("devfs"));

    // create a keyboard device under /dev
    struct stat *status = calloc(1, sizeof(struct stat));
    DevfsRegisterCommand *regcmd = calloc(1, sizeof(DevfsRegisterCommand));
    connections = calloc(MAX_KEYBOARDS, sizeof(int));

    MessageHeader *msgBuffer = calloc(1, SERVER_MAX_SIZE);
    buffer = calloc(KEYBOARD_BUFFER, sizeof(uint16_t));

    if(!status || !regcmd || !connections || !msgBuffer || !buffer) {
        luxLogf(KPRINT_LEVEL_ERROR, "unable to allocate memory for keyboard server\n");
        return -1;
    }

    // character device with permissions r--r--r--
    status->st_mode = (S_IRUSR | S_IRGRP | S_IROTH | S_IFCHR);
    status->st_size = KEYBOARD_BUFFER * sizeof(uint16_t);

    // and register the device
    regcmd->header.command = COMMAND_DEVFS_REGISTER;
    regcmd->header.length = sizeof(DevfsRegisterCommand);
    strcpy(regcmd->path, "/kbd");
    strcpy(regcmd->server, "lux:///dskbd");  // server name prefixed with "lux:///ds"
    memcpy(&regcmd->status, status, sizeof(struct stat));
    luxSendDependency(regcmd);

    free(status);
    free(regcmd);

    // notify lumen that startup is complete
    luxReady();

    for(;;) {
        // accept incoming connections from keyboard drivers
        if(kbdCount < MAX_KEYBOARDS) {
            int sd = luxAccept();
            if(sd > 0) {
                connections[kbdCount] = sd;
                kbdCount++;
            }
        }

        if(!kbdCount) sched_yield();

        // receive key presses
        for(int i = 0; i < kbdCount; i++) {
            ssize_t s = luxRecv(connections[i], msgBuffer, SERVER_MAX_SIZE, false);
            if(s > 0 && s < SERVER_MAX_SIZE) {
                // add the key press to the buffer
                if(bufferSize < KEYBOARD_BUFFER) {
                    buffer[bufferSize] = msgBuffer->status;
                    bufferSize++;
                }
            }
        }

        // and receive read requests
        ssize_t s = luxRecvDependency(msgBuffer, SERVER_MAX_SIZE, false);
        if(s > 0 && s < SERVER_MAX_SIZE) {
            RWCommand *rwcmd = (RWCommand *) msgBuffer;
            size_t rwSize, trueSize;

            if(msgBuffer->command == COMMAND_READ) {
                rwSize = rwcmd->length;
                if(rwSize & 1) rwSize--;    // keypresses take up two bytes
                rwcmd->header.header.response = 1;

                if(!rwSize) {
                    // requested read of size zero
                    rwcmd->header.header.status = 0;
                    rwcmd->header.header.length = sizeof(RWCommand);
                    rwcmd->length = 0;
                } else if(!bufferSize) {
                    // no data available for reading
                    rwcmd->header.header.status = -EWOULDBLOCK;
                    rwcmd->header.header.length = sizeof(RWCommand);
                    rwcmd->length = 0;
                } else {
                    // reading available keypresses
                    rwcmd->header.header.response = 1;
                    if((bufferSize * 2) > rwSize) trueSize = rwSize;
                    else trueSize = bufferSize * 2;

                    // copy the data
                    memcpy(rwcmd->data, buffer, trueSize);
                    rwcmd->header.header.status = trueSize;
                    rwcmd->header.header.length = sizeof(RWCommand) + trueSize;
                    rwcmd->length = trueSize;

                    // clear the keyboard buffer
                    memmove(buffer, (void *)((uintptr_t)buffer + trueSize), KEYBOARD_BUFFER-trueSize);
                    bufferSize -= (trueSize/2);
                }

                // and finally respond to the request
                luxSendDependency(msgBuffer);
            } else {
                luxLogf(KPRINT_LEVEL_WARNING, "undefined command 0x%X, dropping message...\n", msgBuffer->command);
            }
        }
    }
}