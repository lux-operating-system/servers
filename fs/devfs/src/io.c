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

/* devfsRead(): reads from a file on the /dev file system
 * params: req - request buffer
 * params: res - response buffer
 * returns: nothing, response forwarded to vfs server
 */

void devfsRead(SyscallHeader *req, SyscallHeader *res) {
    RWCommand *cmd = (RWCommand *) req;
    memcpy(res, req, sizeof(RWCommand));

    res->header.response = 1;
    res->header.length = sizeof(RWCommand);

    DeviceFile *dev = findDevice(cmd->path);
    if(!dev) {
        res->header.status = -ENOENT;       // file doesn't exist
    } else {
        RWCommand *response = (RWCommand *) res;

        ssize_t status = dev->ioHandler(0, dev->name, &response->position, response->data, cmd->length);
        if(status > 0) res->header.length += status;
        res->header.status = status;
    }

    luxSendDependency(res);
}