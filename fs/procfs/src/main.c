/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * procfs: Microkernel server implementing the /proc file system
 */

#include <procfs/procfs.h>
#include <liblux/liblux.h>
#include <vfs.h>
#include <string.h>
#include <stdlib.h>

int main() {
    luxInit("procfs");
    while(luxConnectDependency("vfs"));     // file system driver

    SyscallHeader *req = calloc(1, SERVER_MAX_SIZE);
    SyscallHeader *res = calloc(1, SERVER_MAX_SIZE);

    if(!req || !res) {
        luxLog(KPRINT_LEVEL_ERROR, "unable to allocate memory for procfs messages\n");
        exit(-1);
    }

    // notify the vfs that this is a file system driver
    VFSInitCommand init;
    memset(&init, 0, sizeof(VFSInitCommand));
    init.header.command = COMMAND_VFS_INIT;
    init.header.length = sizeof(VFSInitCommand);
    init.header.requester = luxGetSelf();
    strcpy(init.fsType, "procfs");
    luxSendDependency(&init);

    // and notify lumen that startup is complete
    luxReady();

    for(;;) {
        // wait for requests from the vfs
        ssize_t s = luxRecvDependency(req, SERVER_MAX_SIZE, false, true);   // peek first
        if(s > 0 && s <= SERVER_MAX_SIZE) {
            if(req->header.length > SERVER_MAX_SIZE) {
                void *newptr = realloc(req, req->header.length);
                if(!newptr) {
                    luxLogf(KPRINT_LEVEL_ERROR, "unable to allocate memory for message handling\n");
                    exit(-1);
                }

                req = newptr;
            }

            luxRecvDependency(req, req->header.length, false, false);

            switch(req->header.command) {
            case COMMAND_MOUNT: procfsMount((MountCommand *) req); break;
            case COMMAND_OPEN: procfsOpen((OpenCommand *) req); break;
            default:
                luxLogf(KPRINT_LEVEL_WARNING, "unimplemented command 0x%X, dropping message...\n", req->header.command);
            }
        }
    }
}