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
#include <devfs/devfs.h>

int main(int argc, char **argv) {
    luxInit("devfs");                   // this will connect to lux and lumen
    while(luxConnectDependency("vfs")); // and to the virtual file system

    // show signs of life
    luxLogf(KPRINT_LEVEL_DEBUG, "devfs server started with pid %d\n", luxGetSelf());

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

    ssize_t s;
    while(1) {
        // idle loop where we just wait for requests from the vfs
        s = luxRecvDependency(req, SERVER_MAX_SIZE, false);
        if(s > 0 && s < SERVER_MAX_SIZE) {
            if(req->header.command >= 0x8000 && req->header.command <= MAX_SYSCALL_COMMAND && dispatchTable[req->header.command&0x7FFF]) {
                dispatchTable[req->header.command&0x7FFF](req, res);
            } else {
                luxLogf(KPRINT_LEVEL_WARNING, "unimplemented command 0x%X for pid %d\n", req->header.command, req->header.requester);
            }
        }
    }
}