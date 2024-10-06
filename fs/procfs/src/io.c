/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * procfs: Microkernel server implementing the /proc file system
 */

#include <procfs/procfs.h>
#include <liblux/liblux.h>
#include <string.h>
#include <stdlib.h>
#include <fnctl.h>
#include <errno.h>

void procfsOpen(OpenCommand *ocmd) {
    pid_t pid;

    ocmd->header.header.response = 1;
    ocmd->header.header.length = sizeof(OpenCommand);

    int res = resolve(ocmd->path, &pid);
    if(res < 0) {
        ocmd->header.header.status = -ENOENT;
        luxSendDependency(ocmd);
        return;
    }

    if(res & RESOLVE_DIRECTORY) {
        ocmd->header.header.status = -EISDIR;
        luxSendDependency(ocmd);
        return;
    }

    if(ocmd->flags & O_WRONLY)
        ocmd->header.header.status = -EPERM;
    else
        ocmd->header.header.status = 0;
    
    luxSendDependency(ocmd);
}