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
#include <vfs.h>

int main(int argc, char **argv) {
    luxInit("devfs");                   // this will connect to lux and lumen
    while(luxConnectDependency("vfs")); // and to the virtual file system

    // show signs of life
    luxLogf(KPRINT_LEVEL_DEBUG, "devfs server started with pid %d\n", getpid());

    SyscallHeader *req = calloc(1, SERVER_MAX_SIZE);
    SyscallHeader *res = calloc(1, SERVER_MAX_SIZE);

    if(!req || !res) {
        luxLog(KPRINT_LEVEL_ERROR, "unable to allocate memory for devfs messages\n");
        while(1) sched_yield();
    }

    // notify the virtual file system that we are a file system driver
    VFSInitCommand init;
    memset(&init, 0, sizeof(VFSInitCommand));
    init.header.command = COMMAND_VFS_INIT;
    init.header.length = sizeof(VFSInitCommand);
    init.header.requester = luxGetSelf();
    strcpy(init.fsType, "devfs");
    luxSendDependency(&init);

    while(1);   // todo
}