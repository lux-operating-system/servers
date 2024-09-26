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
void devfsMount(SyscallHeader *, SyscallHeader *);
void devfsOpen(SyscallHeader *, SyscallHeader *);
void devfsRead(SyscallHeader *, SyscallHeader *);
void devfsWrite(SyscallHeader *, SyscallHeader *);
void devfsIoctl(SyscallHeader *, SyscallHeader *);
void devfsOpendir(SyscallHeader *, SyscallHeader *);
void devfsReaddir(SyscallHeader *, SyscallHeader *);

void (*dispatchTable[])(SyscallHeader *, SyscallHeader *) = {
    devfsStat,          // 0 - stat()
    NULL,               // 1 - flush()
    devfsMount,         // 2 - mount()
    NULL,               // 3 - umount()
    devfsOpen,          // 4 - open()
    devfsRead,          // 5 - read()
    devfsWrite,         // 6 - write()
    devfsIoctl,         // 7 - ioctl()
    devfsOpendir,       // 8 - opendir()
    devfsReaddir,       // 9 - readdir_r()
};