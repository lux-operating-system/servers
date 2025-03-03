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
        luxSendKernel(res);
    } else {
        if(!dev->external) {
            // for devices built-in to the /dev server
            RWCommand *response = (RWCommand *) res;
            ssize_t status = dev->ioHandler(0, dev->name, &response->position, response->data, cmd->length);
            if(status > 0) {
                res->header.length += status;
                response->length = status;
            } else {
                req->header.length = 0;
            }

            res->header.status = status;
            luxSendKernel(res);
        } else {
            // for external devices, relay the request again
            driverRead(cmd, dev);
        }
    }
}

/* devfsWrite(): writes to a file on the /dev file system
 * params: req - request buffer
 * params: res - response buffer
 * returns: nothing, response forwarded to vfs server
 */

void devfsWrite(SyscallHeader *req, SyscallHeader *res) {
    RWCommand *cmd = (RWCommand *) req;
    memcpy(res, req, sizeof(RWCommand));

    res->header.response = 1;
    res->header.length = sizeof(RWCommand);

    DeviceFile *dev = findDevice(cmd->path);
    if(!dev) {
        res->header.status = -ENOENT;       // file doesn't exist
        luxSendKernel(res);
    } else {
        if(!dev->external) {
            // for devices built-in to the /dev server
            RWCommand *response = (RWCommand *) res;
            ssize_t status = dev->ioHandler(1, dev->name, &response->position, response->data, cmd->length);
            res->header.length = sizeof(RWCommand);

            if(status > 0) response->length = status;
            else req->header.length = 0;

            res->header.status = status;
            luxSendKernel(res);
        } else {
            // for external devices, relay the request again
            driverWrite(cmd, dev);
        }
    }
}

/* devfsMmap(): handler for mmap() on the /dev file system
 * params: req - request buffer
 * params: res - response buffer
 * returns: nothing, response forwarded to vfs server
 */

void devfsMmap(SyscallHeader *req, SyscallHeader *res) {
    MmapCommand *cmd = (MmapCommand *) req;
    memcpy(res, req, sizeof(MmapCommand));

    res->header.response = 1;
    res->header.length = sizeof(MmapCommand);

    DeviceFile *dev = findDevice(cmd->path);
    if(!dev) {
        res->header.status = -ENOENT;
        luxSendKernel(res);
    } else if(dev->external) {
        luxSend(dev->socket, cmd);
    } else {
        // mmap() is only supported for external drivers and not by the default
        // devices (null, random, etc) provided by this server
        res->header.status = -ENODEV;
        luxSendKernel(res);
    }
}

/* devfsFsync(): handler for fsync() on the /dev file system
 * params: req - request buffer
 * params: res - response buffer
 * returns: nothing, response relayed to kernel
 */

void devfsFsync(SyscallHeader *req, SyscallHeader *res) {
    FsyncCommand *cmd = (FsyncCommand *) req;
    memcpy(res, req, sizeof(FsyncCommand));

    res->header.response = 1;
    res->header.length = sizeof(FsyncCommand);

    DeviceFile *dev = findDevice(cmd->path);
    if(!dev) {
        res->header.status = -ENOENT;       // file doesn't exist
        luxSendKernel(res);
    } else {
        if(!dev->external) {
            // for devices built-in to the /dev server there isn't anything to do
            res->header.status = 0;
            luxSendKernel(res);
        } else {
            // for external devices, relay the request again
            luxSend(dev->socket, req);
        }
    }
}