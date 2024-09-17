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

void (*dispatchTable[])(SyscallHeader *, SyscallHeader *) = {
    devfsStat,          // 0 - stat()
    NULL,               // 1 - flush()
    devfsMount,         // 2 - mount()
    NULL,               // 3 - umount()
    NULL,               // 4 - open()
    NULL,               // 5 - read()
    NULL,               // 6 - write()
};