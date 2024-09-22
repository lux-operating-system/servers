/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * pty: Microkernel server implementing Unix 98-style pseudo-terminal devices
 */

#pragma once

#include <liblux/liblux.h>

#define MAX_PTYS                4096

typedef struct {
    int valid, index, openCount;
} Pty;

extern Pty *ptys;
extern int ptyCount;

void ptyOpen(OpenCommand *);
void ptyOpenMaster(OpenCommand *);
void ptyOpenSlave(OpenCommand *);
