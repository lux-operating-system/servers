/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * devfs: Microkernel server implementing the /dev file system
 */

#include <liblux/liblux.h>
#include <devfs/devfs.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

/* devfsOpendir(): handler for opendir() on the /dev file system
 * params: req - request message buffer
 * params: res - response message buffer
 * returns: nothing, response relayed to virtual file system
 */

void devfsOpendir(SyscallHeader *req, SyscallHeader *res) {
    OpendirCommand *cmd = (OpendirCommand *) req;
    memcpy(res, req, sizeof(OpendirCommand));

    res->header.response = 1;

    DeviceFile *file = findDevice(cmd->path);
    if(!file) {
        res->header.status = -ENOENT;   // directory doesn't exist
        luxSendDependency(res);
        return;
    }

    // this syscall is only valid for directories
    if((file->status.st_mode & S_IFMT) != S_IFDIR) {
        res->header.status = -ENOTDIR;
        luxSendDependency(res);
        return;
    }

    // ensure the requesting process has adequate privileges
    // the execute permission on a directory is used to indicate permission for
    // directory listing, which is exactly what opendir() is used for
    res->header.status = 0;
    if(cmd->uid == file->status.st_uid)         // owner
        if(!(file->status.st_mode & S_IXUSR)) res->header.status = -EACCES;
    else if(cmd->gid == file->status.st_gid)    // group
        if(!(file->status.st_mode & S_IXGRP)) res->header.status = -EACCES;
    else                                        // other
        if(!(file->status.st_mode & S_IXOTH)) res->header.status = -EACCES;
    
    // and relay the response to the virtual file system
    luxSendDependency(res);
}