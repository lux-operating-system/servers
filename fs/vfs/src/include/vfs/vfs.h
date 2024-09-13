/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * vfs: Microkernel server implementing a virtual file system
 */

#include <liblux/liblux.h>
#include <vfs.h>

extern void (*vfsDispatchTable[])(SyscallHeader *);
extern FileSystemServers *servers;
extern int serverCount;

int findFSServer(const char *);
