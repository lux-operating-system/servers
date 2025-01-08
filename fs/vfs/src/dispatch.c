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

void vfsDispatchChmod(SyscallHeader *hdr) {
    ChmodCommand *cmd = (ChmodCommand *) hdr;
    char type[32];
    if(resolve(cmd->path, type, cmd->device, cmd->path)) {
        int sd = findFSServer(type);
        if(sd <= 0) luxLogf(KPRINT_LEVEL_WARNING, "no file system driver loaded for '%s'\n", type);
        else luxSend(sd, cmd);
    } else {
        luxLogf(KPRINT_LEVEL_WARNING, "could not resolve path '%s'\n", cmd->path);
    }
}

void vfsDispatchChown(SyscallHeader *hdr) {
    ChownCommand *cmd = (ChownCommand *) hdr;
    char type[32];
    if(resolve(cmd->path, type, cmd->device, cmd->path)) {
        int sd = findFSServer(type);
        if(sd <= 0) luxLogf(KPRINT_LEVEL_WARNING, "no file system driver loaded for '%s'\n", type);
        else luxSend(sd, cmd);
    } else {
        luxLogf(KPRINT_LEVEL_WARNING, "could not resolve path '%s'\n", cmd->path);
    }
}

void vfsDispatchLink(SyscallHeader *hdr) {
    LinkCommand *cmd = (LinkCommand *) hdr;
    char type[32];
    char device[MAX_FILE_PATH];
    char *ptr = resolve(cmd->newPath, type, cmd->device, cmd->newPath);
    if(ptr) ptr = resolve(cmd->oldPath, type, device, cmd->oldPath);
    if(ptr) {
        if(strcmp(cmd->device, device)) {
            // https://pubs.opengroup.org/onlinepubs/9799919799/functions/link.html
            // linking between different file systems is an optional feature
            // and so until i see a clean way to implement them, we'll skip this
            cmd->header.header.response = 1;
            cmd->header.header.status = -EXDEV;
            luxSendKernel(cmd);
            return;
        }

        int sd = findFSServer(type);
        if(sd <= 0) luxLogf(KPRINT_LEVEL_WARNING, "no file system driver loaded for '%s'\n", type);
        else luxSend(sd, cmd);
    } else {
        luxLogf(KPRINT_LEVEL_WARNING, "could not resolve paths '%s', '%s'\n", cmd->newPath, cmd->oldPath);
    }
}

void vfsDispatchMkdir(SyscallHeader *hdr) {
    MkdirCommand *cmd = (MkdirCommand *) hdr;
    char type[32];
    if(resolve(cmd->path, type, cmd->device, cmd->path)) {
        int sd = findFSServer(type);
        if(sd <= 0) luxLogf(KPRINT_LEVEL_WARNING, "no file system driver loaded for '%s'\n", type);
        else luxSend(sd, cmd);
    } else {
        luxLogf(KPRINT_LEVEL_WARNING, "could not resolve path '%s'\n", cmd->path);
    }
}

void vfsDispatchUtime(SyscallHeader *hdr) {
    UtimeCommand *cmd = (UtimeCommand *) hdr;
    char type[32];
    if(resolve(cmd->path, type, cmd->device, cmd->path)) {
        int sd = findFSServer(type);
        if(sd <= 0) luxLogf(KPRINT_LEVEL_WARNING, "no file system driver loaded for '%s'\n", type);
        else luxSend(sd, cmd);
    } else {
        luxLogf(KPRINT_LEVEL_WARNING, "could not resolve path '%s'\n", cmd->path);
    }
}

void vfsDispatchUnlink(SyscallHeader *hdr) {
    UnlinkCommand *cmd = (UnlinkCommand *) hdr;
    char type[32];
    if(resolve(cmd->path, type, cmd->device, cmd->path)) {
        int sd = findFSServer(type);
        if(sd <= 0) luxLogf(KPRINT_LEVEL_WARNING, "no file system driver loaded for '%s'\n", type);
        else luxSend(sd, cmd);
    } else {
        luxLogf(KPRINT_LEVEL_WARNING, "could not resolve path '%s'\n", cmd->path);
    }
}

void vfsDispatchSymlink(SyscallHeader *hdr) {
    LinkCommand *cmd = (LinkCommand *) hdr;
    char type[32];
    if(resolve(cmd->newPath, type, cmd->device, cmd->newPath)) {
        int sd = findFSServer(type);
        if(sd <= 0) luxLogf(KPRINT_LEVEL_WARNING, "no file system driver loaded for '%s'\n", type);
        else luxSend(sd, cmd);
    } else {
        luxLogf(KPRINT_LEVEL_WARNING, "could not resolve path '%s'\n", cmd->newPath);
    }
}

void vfsDispatchReadLink(SyscallHeader *hdr) {
    ReadLinkCommand *cmd = (ReadLinkCommand *) hdr;
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
    vfsDispatchChmod,   // 10 - chmod()
    vfsDispatchChown,   // 11 - chown()
    vfsDispatchLink,    // 12 - link()
    vfsDispatchMkdir,   // 13 - mkdir()
    vfsDispatchUtime,   // 14 - utime()

    NULL, NULL, NULL,   // 15, 16, 17 - irrelevant to vfs

    vfsDispatchMmap,    // 18 - mmap()
    NULL,               // 19 - msync()
    vfsDispatchUnlink,  // 20 - unlink()
    vfsDispatchSymlink, // 21 - symlink()
    vfsDispatchReadLink // 22 - readlink()
};