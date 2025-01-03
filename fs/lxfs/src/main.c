/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024-25
 * 
 * lxfs: Driver for the lxfs file system
 */

#include <liblux/liblux.h>
#include <lxfs/lxfs.h>
#include <vfs.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

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
        ssize_t s = luxRecvCommand((void **) &msg);
        if(s > 0) {
            switch(msg->header.command) {
            case COMMAND_MOUNT: lxfsMount((MountCommand *) msg); break;
            case COMMAND_OPEN: lxfsOpen((OpenCommand *) msg); break;
            case COMMAND_READ: lxfsRead((RWCommand *) msg); break;
            case COMMAND_WRITE: lxfsWrite((RWCommand *) msg); break;
            case COMMAND_STAT: lxfsStat((StatCommand *) msg); break;
            case COMMAND_OPENDIR: lxfsOpendir((OpendirCommand *) msg); break;
            case COMMAND_READDIR: lxfsReaddir((ReaddirCommand *) msg); break;
            case COMMAND_MMAP: lxfsMmap((MmapCommand *) msg); break;
            case COMMAND_CHMOD: lxfsChmod((ChmodCommand *) msg); break;
            case COMMAND_CHOWN: lxfsChown((ChownCommand *) msg); break;
            default:
                luxLogf(KPRINT_LEVEL_WARNING, "unimplemented command 0x%04X, dropping message...\n", msg->header.command);
                msg->header.status = -ENOSYS;
                luxSendKernel(msg);
            }
        } else {
            sched_yield();
        }
    }
}