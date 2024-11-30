/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * vfs: Microkernel server implementing a virtual file system
 */

#include <liblux/liblux.h>
#include <vfs.h>
#include <vfs/vfs.h>
#include <errno.h>
#include <string.h>

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

void vfsDispatchWrite(SyscallHeader *hdr) {
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

void vfsDispatchIoctl(SyscallHeader *hdr) {
    IOCTLCommand *cmd = (IOCTLCommand *) hdr;
    char type[32];
    if(resolve(cmd->path, type, cmd->device, cmd->path)) {
        // ioctl() is only valid for /dev because it manipulates device files
        if(strcmp(type, "devfs")) {
            cmd->header.header.length = sizeof(IOCTLCommand);
            cmd->header.header.response = 1;
            cmd->header.header.status = -ENOTTY;
            luxSendLumen(cmd);
            return;
        }

        int sd = findFSServer(type);
        if(sd <= 0) luxLogf(KPRINT_LEVEL_WARNING, "no file system driver loaded for '%s'\n", type);
        else luxSend(sd, cmd);
    } else {
        luxLogf(KPRINT_LEVEL_WARNING, "could not resolve path '%s'\n", cmd->path);
    }
}

void vfsDispatchOpendir(SyscallHeader *hdr) {
    OpendirCommand *cmd = (OpendirCommand *) hdr;
    char type[32];
    if(resolve(cmd->path, type, cmd->device, cmd->abspath)) {
        int sd = findFSServer(type);
        if(sd <= 0) luxLogf(KPRINT_LEVEL_WARNING, "no file system driver loaded for '%s'\n", type);
        else luxSend(sd, cmd);
    } else {
        luxLogf(KPRINT_LEVEL_WARNING, "could not resolve path '%s'\n", cmd->abspath);
    }
}

void vfsDispatchReaddir(SyscallHeader *hdr) {
    ReaddirCommand *cmd = (ReaddirCommand *) hdr;
    char type[32];
    if(resolve(cmd->path, type, cmd->device, cmd->path)) {
        int sd = findFSServer(type);
        if(sd <= 0) luxLogf(KPRINT_LEVEL_WARNING, "no file system driver loaded for '%s'\n", type);
        else luxSend(sd, cmd);
    } else {
        luxLogf(KPRINT_LEVEL_WARNING, "could not resolve path '%s'\n", cmd->path);
    }
}

void vfsDispatchMmap(SyscallHeader *hdr) {
    MmapCommand *cmd = (MmapCommand *) hdr;
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
    vfsDispatchWrite,   // 6 - write()
    vfsDispatchIoctl,   // 7 - ioctl()
    vfsDispatchOpendir, // 8 - opendir()
    vfsDispatchReaddir, // 9 - readdir_r()
    NULL,               // 10 - chmod()
    NULL,               // 11 - chown()
    NULL,               // 12 - link()
    NULL,               // 13 - mkdir()
    NULL,               // 14 - rmdir()

    NULL, NULL, NULL,   // 15, 16, 17 - irrelevant to vfs

    vfsDispatchMmap,    // 18 - mmap()
    NULL,               // 19 - msync()
};