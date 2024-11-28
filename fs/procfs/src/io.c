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
#include <fcntl.h>
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
    else if(res == RESOLVE_CPU) scmd->buffer.st_size = strlen(sysinfo->cpu);
    else scmd->buffer.st_size = 8;

    luxSendDependency(scmd);
}

void procfsRead(RWCommand *rcmd) {
    rcmd->header.header.response = 1;
    rcmd->header.header.length = sizeof(RWCommand);

    pid_t pid;
    int file = resolve(rcmd->path, &pid);
    if(file < 0) {
        rcmd->header.header.status = -ENOENT;
        luxSendDependency(rcmd);
        return;
    }

    RWCommand *res = calloc(1, sizeof(RWCommand) + rcmd->length);
    if(!res) {
        rcmd->header.header.status = -ENOMEM;
        rcmd->length = 0;
        luxSendDependency(rcmd);
        return;
    }

    memcpy(res, rcmd, sizeof(RWCommand));

    uint64_t data;
    void *ptr = (void *) &data;
    size_t size = 8;

    switch(file) {
    case RESOLVE_KERNEL:
        ptr = sysinfo->kernel;
        size = strlen(sysinfo->kernel);
        break;
    case RESOLVE_CPU:
        ptr = sysinfo->cpu;
        size = strlen(sysinfo->cpu);
        break;
    case RESOLVE_MEMSIZE:
        luxSysinfo(sysinfo);
        data = sysinfo->memorySize;
        size = 4;
        break;
    case RESOLVE_MEMUSAGE:
        luxSysinfo(sysinfo);
        data = sysinfo->memoryUsage;
        size = 4;
        break;
    case RESOLVE_UPTIME:
        luxSysinfo(sysinfo);
        data = sysinfo->uptime;
        break;
    default:
        rcmd->header.header.status = -ENOENT;
        rcmd->length = 0;
        luxSendDependency(rcmd);
        free(res);
        return;
    }

    if(rcmd->position >= size) {
        rcmd->header.header.status = -EOVERFLOW;
        rcmd->length = 0;
        luxSendDependency(rcmd);
        free(res);
        return;
    }

    size_t truelen;
    if((rcmd->position + rcmd->length) > size) truelen = size - rcmd->position;
    else truelen = size;

    memcpy(res->data, ptr + rcmd->position, truelen);
    res->length = truelen;
    res->header.header.status = truelen;
    res->header.header.length += truelen;
    res->position += truelen;
    luxSendDependency(res);
    free(res);
}