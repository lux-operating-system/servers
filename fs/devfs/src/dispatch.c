/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * devfs: Microkernel server implementing the /dev file system
 */

#include <liblux/liblux.h>
#include <vfs.h>
#include <devfs/devfs.h>

void mount(SyscallHeader *, SyscallHeader *);

void (*dispatchTable[])(SyscallHeader *, SyscallHeader *) = {
    NULL,               // 0 - stat()
    NULL,               // 1 - flush()
    mount,              // 2 - mount()
    NULL,               // 3 - umount()
};