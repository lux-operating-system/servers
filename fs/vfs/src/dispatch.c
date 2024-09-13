/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * vfs: Microkernel server implementing a virtual file system
 */

#include <liblux/liblux.h>
#include <vfs.h>
#include <vfs/vfs.h>

void vfsDispatchMount(SyscallHeader *hdr) {
    MountCommand *cmd = (MountCommand *) hdr;
    luxLogf(KPRINT_LEVEL_DEBUG, "mounting file system '%s' at '%s'\n", cmd->type, cmd->target);

    int sd = findFSServer(cmd->type);
    if(sd <= 0) luxLogf(KPRINT_LEVEL_WARNING, "no file system driver loaded for '%s'\n", cmd->type);
    else luxSend(sd, cmd);
}

void (*vfsDispatchTable[])(SyscallHeader *) = {
    NULL,               // 0 - stat()
    NULL,               // 1 - flush()
    vfsDispatchMount,   // 2 - mount()
    NULL,               // 3 - umount()
};