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

    if(strlen(cmd->path) == 1 && cmd->path[0] == '/') {
        res->header.status = 0;
        luxSendDependency(res);
        return;
    }

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

/* countPath(): helper function to count the depth of a directory
 * params: path - path to count
 * returns: number of backslashes in the path
 */

static int countPath(const char *path) {
    if(!path || !strlen(path)) return 0;
    if(strlen(path) == 1) return 0;
    int c = 0;
    for(int i = 0; i < strlen(path); i++)
        if(path[i] == '/') c++;

    return c;
}

/* devfsReaddir(): handler for readdir_r() on the /dev file system
 * params: req - request message buffer
 * params: res - response message buffer
 * returns: nothing, response relayed to virtual file system
 */

void devfsReaddir(SyscallHeader *req, SyscallHeader *res) {
    ReaddirCommand *cmd = (ReaddirCommand *) req;
    memcpy(res, req, sizeof(ReaddirCommand));

    res->header.response = 1;
    res->header.length = sizeof(ReaddirCommand);

    ReaddirCommand *response = (ReaddirCommand *) res;

    if(!cmd->position) {
        strcpy(response->entry.d_name, ".");    // current directory
        response->position++;
        response->header.header.status = 0;
        response->end = 0;
        luxSendDependency(res);
        return;
    } else if(cmd->position == 1) {
        strcpy(response->entry.d_name, "..");   // parent directory
        response->position++;
        response->header.header.status = 0;
        response->end = 0;
        luxSendDependency(res);
        return;
    }

    size_t position = cmd->position - 2;    // skipping over self and parent
    int depth = countPath(cmd->path);
    int parentLength = strlen(cmd->path);
    size_t counter = 0;

    for(int i = 0; i < deviceCount; i++) {
        if((!memcmp(devices[i].name, cmd->path, parentLength)) && (countPath(devices[i].name) == (depth+1))) {
            counter++;
            if(counter > position) {
                if(parentLength > 1) strcpy(response->entry.d_name, &devices[i].name[parentLength+1]);
                else strcpy(response->entry.d_name, &devices[i].name[1]);

                response->position++;
                response->header.header.status = 0;
                response->end = 0;
                luxSendDependency(res);
                return;
            }
        }
    }

    // reached end of directory
    response->header.header.status = 0;
    response->end = 1;
    luxSendDependency(res);
}