/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * procfs: Microkernel server implementing the /proc file system
 */

#pragma once

#include <liblux/liblux.h>

void procfsMount(MountCommand *, MountCommand *);
