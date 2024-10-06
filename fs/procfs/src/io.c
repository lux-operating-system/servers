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
#include <sys/stat.h>

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

void procfsStat(StatCommand *scmd) {
    scmd->header.header.response = 1;
    scmd->header.header.length = sizeof(StatCommand);

    pid_t pid;
    int res = resolve(scmd->path, &pid);
    if(res < 0) {
        scmd->header.header.status = -ENOENT;
        luxSendDependency(scmd);
        return;
    }

    scmd->header.header.status = 0;

    memset(&scmd->buffer, 0, sizeof(struct stat));
    scmd->buffer.st_mode = S_IRUSR | S_IRGRP | S_IROTH;
    if(res & RESOLVE_DIRECTORY) scmd->buffer.st_mode |= S_IFDIR;

    if(res == RESOLVE_KERNEL) scmd->buffer.st_size = strlen(sysinfo->kernel);
    else scmd->buffer.st_size = 8;

    luxSendDependency(scmd);
}