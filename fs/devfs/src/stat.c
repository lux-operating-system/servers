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

    DeviceFile *dev = findDevice(cmd->path);
    if(!dev) {
        res->header.status = -ENOENT;   // file doesn't exist
    } else {
        StatCommand *response = (StatCommand *) res;
        response->header.header.status = 0;
        memcpy(&response->buffer, &dev->status, sizeof(struct stat));
    }

    luxSendDependency(res);
}
