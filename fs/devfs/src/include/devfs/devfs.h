/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * devfs: Microkernel server implementing the /dev file system
 */

#pragma once

#include <liblux/liblux.h>

extern void (*dispatchTable[])(SyscallHeader *, SyscallHeader *);
