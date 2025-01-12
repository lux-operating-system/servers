/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * devfs: Microkernel server implementing the /dev file system
 */

#include <liblux/liblux.h>
#include <vfs.h>
#include <devfs/devfs.h>

void devfsStat(SyscallHeader *, SyscallHeader *);
void devfsFsync(SyscallHeader *, SyscallHeader *);
void devfsMount(SyscallHeader *, SyscallHeader *);
void devfsOpen(SyscallHeader *, SyscallHeader *);
void devfsRead(SyscallHeader *, SyscallHeader *);
void devfsWrite(SyscallHeader *, SyscallHeader *);
void devfsIoctl(SyscallHeader *, SyscallHeader *);
void devfsOpendir(SyscallHeader *, SyscallHeader *);
void devfsReaddir(SyscallHeader *, SyscallHeader *);
void devfsMmap(SyscallHeader *, SyscallHeader *);

void (*dispatchTable[])(SyscallHeader *, SyscallHeader *) = {
    devfsStat,          // 0 - stat()
    devfsFsync,         // 1 - fsync()
    devfsMount,         // 2 - mount()
    NULL,               // 3 - umount()
    devfsOpen,          // 4 - open()
    devfsRead,          // 5 - read()
    devfsWrite,         // 6 - write()
    devfsIoctl,         // 7 - ioctl()
    devfsOpendir,       // 8 - opendir()
    devfsReaddir,       // 9 - readdir_r()
    NULL,               // 10 - chmod()
    NULL,               // 11 - chown()
    NULL,               // 12 - link()
    NULL,               // 13 - mkdir()
    NULL,               // 14 - rmdir()

    NULL, NULL, NULL,   // 15, 16, 17 - irrelevant to devfs

    devfsMmap,          // 18 - mmap()
    NULL,               // 19 - msync()
    NULL,               // 20 - unlink()
    NULL,               // 21 - symlink()
    NULL,               // 22 - readlink()
};