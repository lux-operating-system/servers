/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * pty: Microkernel server implementing Unix 98-style pseudo-terminal devices
 */

#pragma once

#include <liblux/liblux.h>
#include <sys/types.h>

#define MAX_PTYS                4096
#define PTY_BUFFER_INCREMENTS   4096

typedef struct {
    // master read() will read from slave, write() will write to master
    // slave read() will read from master, write() will write to slave
    int valid, index, openCount;
    void *master, *slave;
    size_t masterSize, slaveSize;
} Pty;

extern Pty *ptys;
extern int ptyCount;

void ptyOpen(OpenCommand *);
void ptyOpenMaster(OpenCommand *);
void ptyOpenSlave(OpenCommand *);
