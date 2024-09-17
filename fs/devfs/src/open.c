/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * devfs: Microkernel server implementing the /dev file system
 */

#include <errno.h>
#include <string.h>
#include <fnctl.h>
#include <liblux/liblux.h>
#include <vfs.h>
#include <devfs/devfs.h>

void devfsOpen(SyscallHeader *req, SyscallHeader *res) {
    OpenCommand *cmd = (OpenCommand *) req;
    memcpy(res, req, req->header.length);
    res->header.response = 1;

    DeviceFile *file = findDevice(cmd->path);
    if(!file) {
        res->header.status = -ENOENT;   // file doesn't exist
        luxSendDependency(res);
    }

    // make sure the requester has the appropriate permissions
    res->header.status = 0;     // "success"

    if(cmd->uid == file->status.st_uid) {
        // owner
        if(((cmd->flags & O_RDONLY) && !(file->status.st_mode & S_IRUSR))) res->header.status = -EACCES;
        if(((cmd->flags & O_WRONLY) && !(file->status.st_mode & S_IWUSR))) res->header.status = -EACCES;
    } else if(cmd->gid == file->status.st_gid) {
        // group
        if(((cmd->flags & O_RDONLY) && !(file->status.st_mode & S_IRGRP))) res->header.status = -EACCES;
        if(((cmd->flags & O_WRONLY) && !(file->status.st_mode & S_IWGRP))) res->header.status = -EACCES;
    } else {
        // other
        if(((cmd->flags & O_RDONLY) && !(file->status.st_mode & S_IROTH))) res->header.status = -EACCES;
        if(((cmd->flags & O_WRONLY) && !(file->status.st_mode & S_IWOTH))) res->header.status = -EACCES;
    }

    luxSendDependency(res);
}