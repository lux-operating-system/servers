/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * devfs: Microkernel server implementing the /dev file system
 */

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <liblux/liblux.h>

int main(int argc, char **argv) {
    luxInit("devfs");     // this will connect to lux and lumen
    while(!luxConnectDependency("vfs"));

    // show signs of life
    luxLogf(KPRINT_LEVEL_DEBUG, "devfs server started with pid %d\n", getpid());

    SyscallHeader *req = calloc(1, SERVER_MAX_SIZE);
    SyscallHeader *res = calloc(1, SERVER_MAX_SIZE);

    if(!req || !res) {
        luxLog(KPRINT_LEVEL_ERROR, "unable to allocate memory for devfs messages\n");
        while(1) sched_yield();
    }

    while(1);   // todo
}