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

    // and wait for acknowledgement
    ssize_t rs = luxRecvDependency(&init, sizeof(VFSInitCommand), true, false);
    if(rs < sizeof(VFSInitCommand) || init.header.command != COMMAND_VFS_INIT
    || init.header.status) {
        luxLogf(KPRINT_LEVEL_ERROR, "failed to register file system driver\n");
        for(;;);
    }

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
            case COMMAND_MKDIR: lxfsMkdir((MkdirCommand *) msg); break;
            case COMMAND_UTIME: lxfsUtime((UtimeCommand *) msg); break;
            case COMMAND_LINK: lxfsLink((LinkCommand *) msg); break;
            case COMMAND_UNLINK: lxfsUnlink((UnlinkCommand *) msg); break;
            case COMMAND_SYMLINK: lxfsSymlink((LinkCommand *) msg); break;
            case COMMAND_READLINK: lxfsReadLink((ReadLinkCommand *) msg); break;
            case COMMAND_FSYNC: lxfsFsync((FsyncCommand *) msg); break;
            case COMMAND_STATVFS: lxfsStatvfs((StatvfsCommand *) msg); break;
            default:
                msg->header.response = 1;
                msg->header.status = -ENOSYS;
                luxSendKernel(msg);
            }
        } else {
            sched_yield();
        }
    }
}