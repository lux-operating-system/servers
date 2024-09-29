/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * lxfs: Driver for the lxfs file system
 */

#include <liblux/liblux.h>
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

    for(;;);
}