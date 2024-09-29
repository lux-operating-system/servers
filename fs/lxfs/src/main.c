/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * lxfs: Driver for the lxfs file system
 */

#include <liblux/liblux.h>
#include <lxfs/lxfs.h>
#include <vfs.h>
#include <string.h>
#include <stdlib.h>

int main() {
    luxInit("lxfs");
    while(luxConnectDependency("vfs"));

    SyscallHeader *msg = calloc(1, SERVER_MAX_SIZE);

    if(!msg) {
        luxLogf(KPRINT_LEVEL_ERROR, "unable to allocate memory\n");
        return -1;
    }

    // notify the vfs that we are a file system driver
    VFSInitCommand init;
    memset(&init, 0, sizeof(VFSInitCommand));
    init.header.command = COMMAND_VFS_INIT;
    init.header.length = sizeof(VFSInitCommand);
    init.header.requester = luxGetSelf();
    strcpy(init.fsType, "lxfs");
    luxSendDependency(&init);

    // and notify lumen that startup is complete
    luxReady();

    for(;;) {
        // handle requests here
        ssize_t s = luxRecvDependency(msg, SERVER_MAX_SIZE, false, true);   // peek first
        if(s > 0 && s <= SERVER_MAX_SIZE) {
            if(msg->header.length > SERVER_MAX_SIZE) {
                void *newptr = realloc(msg, msg->header.length);
                if(!newptr) {
                    luxLogf(KPRINT_LEVEL_ERROR, "failed to allocate memory for message handling\n");
                    exit(-1);
                }

                msg = newptr;
            }

            luxRecvDependency(msg, msg->header.length, false, false);

            switch(msg->header.command) {
            case COMMAND_MOUNT: lxfsMount((MountCommand *) msg); break;
            default:
                luxLogf(KPRINT_LEVEL_WARNING, "unimplemented command 0x%04X, dropping message...\n", msg->header.command);
            }
        }
    }
}