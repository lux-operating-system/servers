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

void vfsDispatchStat(SyscallHeader *hdr) {
    StatCommand *cmd = (StatCommand *) hdr;
    char type[32];
    if(resolve(cmd->path, type, cmd->source, cmd->path)) {
        int sd = findFSServer(type);
        if(sd <= 0) luxLogf(KPRINT_LEVEL_WARNING, "no file system driver loaded for '%s'\n", type);
        else luxSend(sd, cmd);
    } else {
        luxLogf(KPRINT_LEVEL_WARNING, "could not resolve path '%s'\n", cmd->path);
    }
}

void vfsDispatchOpen(SyscallHeader *hdr) {
    OpenCommand *cmd = (OpenCommand *) hdr;
    char type[32];
    if(resolve(cmd->path, type, cmd->device, cmd->abspath)) {
        int sd = findFSServer(type);
        if(sd <= 0) luxLogf(KPRINT_LEVEL_WARNING, "no file system driver loaded for '%s'\n", type);
        else luxSend(sd, cmd);
    } else {
        luxLogf(KPRINT_LEVEL_WARNING, "could not resolve path '%s'\n", cmd->path);
    }
}

void vfsDispatchRead(SyscallHeader *hdr) {
    RWCommand *cmd = (RWCommand *) hdr;
    char type[32];
    if(resolve(cmd->path, type, cmd->device, cmd->path)) {
        int sd = findFSServer(type);
        if(sd <= 0) luxLogf(KPRINT_LEVEL_WARNING, "no file system driver loaded for '%s'\n", type);
        else luxSend(sd, cmd);
    } else {
        luxLogf(KPRINT_LEVEL_WARNING, "could not resolve path '%s'\n", cmd->path);
    }
}

void (*vfsDispatchTable[])(SyscallHeader *) = {
    vfsDispatchStat,    // 0 - stat()
    NULL,               // 1 - flush()
    vfsDispatchMount,   // 2 - mount()
    NULL,               // 3 - umount()
    vfsDispatchOpen,    // 4 - open()
    vfsDispatchRead,    // 5 - read()
    NULL,               // 6 - write()
};