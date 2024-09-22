/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * devfs: Microkernel server implementing the /dev file system
 */

#include <liblux/liblux.h>
#include <liblux/devfs.h>
#include <devfs/devfs.h>
#include <string.h>
#include <errno.h>

/* devfsIoctl(): handles ioctl() for device files
 * params: req - request buffer
 * params: res - response buffer
 * returns: nothing
 */

void devfsIoctl(SyscallHeader *req, SyscallHeader *res) {
    IOCTLCommand *cmd = (IOCTLCommand *) req;
    memcpy(res, req, sizeof(IOCTLCommand));

    res->header.response = 1;
    res->header.length = sizeof(IOCTLCommand);

    DeviceFile *dev = findDevice(cmd->path);
    if(!dev) {
        // file doesn't exist
        res->header.status = -ENOENT;
        luxSendDependency(res);
        return;
    }

    // ioctl() is only valid for character special devices, and the internal
    // devices implemented in this server don't implement any ioctl() opcodes,
    // so this is also only valid for external drivers
    if(((dev->status.st_mode & S_IFMT) != S_IFCHR) || (!dev->external)) {
        res->header.status = -ENOTTY;
        luxSendDependency(res);
        return;
    }

    // relay the call to the external driver
    luxSend(dev->socket, cmd);
}
