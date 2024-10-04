/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * devfs: Microkernel server implementing the /dev file system
 */

#include <string.h>
#include <errno.h>
#include <liblux/liblux.h>
#include <vfs.h>
#include <devfs/devfs.h>

/* devfsStat(): returns the file status of a file on the /dev file system
 * params: req - request buffer
 * params: res - response buffer
 * returns: nothing, response forwarded to vfs server
 */

void devfsStat(SyscallHeader *req, SyscallHeader *res) {
    StatCommand *cmd = (StatCommand *) req;
    memcpy(res, req, req->header.length);

    res->header.response = 1;
    StatCommand *response = (StatCommand *) res;

    if((strlen(cmd->path) == 1) && (cmd->path[0] == '/')) {
        // /dev is owned by root:root, rwxr-xr-x
        response->header.header.status = 0;
        response->buffer.st_mode = S_IFDIR | S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
        response->buffer.st_uid = 0;
        response->buffer.st_gid = 0;
        response->buffer.st_size = deviceCount;
        luxSendDependency(response);

        return;
    }

    DeviceFile *dev = findDevice(cmd->path);
    if(!dev) {
        res->header.status = -ENOENT;   // file doesn't exist
    } else {
        response->header.header.status = 0;
        memcpy(&response->buffer, &dev->status, sizeof(struct stat));
    }

    luxSendDependency(res);
}
