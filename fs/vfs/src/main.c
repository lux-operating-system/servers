/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * vfs: Microkernel server implementing a virtual file system
 */

#include <stdlib.h>
#include <unistd.h>
#include <liblux/liblux.h>

int main(int argc, char **argv) {
    luxInit("vfs");     // this will connect to lux and lumen

    // show signs of life
    luxLog(KPRINT_LEVEL_DEBUG, "virtual file system server started\n");

    SyscallHeader *req = calloc(1, SERVER_MAX_SIZE);
    SyscallHeader *res = calloc(1, SERVER_MAX_SIZE);

    if(!req || !res) {
        luxLog(KPRINT_LEVEL_ERROR, "unable to allocate memory for vfs messages\n");
        while(1) sched_yield();
    }

    ssize_t s;
    while(1) {
        // wait for incoming requests
        s = luxRecvLumen(req, SERVER_MAX_SIZE, true);
        if(s > 0) {
            // stub for testing
            luxLogf(KPRINT_LEVEL_DEBUG, "received syscall request 0x%X for pid %d\n", req->header.command, req->header.requester);
            req->header.length = sizeof(SyscallHeader);
            req->header.response = 1;
            req->header.status = 0;     // "success"
            luxSendLumen(req);
        }
    }
}