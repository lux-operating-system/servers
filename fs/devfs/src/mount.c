/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * devfs: Microkernel server implementing the /dev file system
 */

#include <string.h>
#include <liblux/liblux.h>
#include <vfs.h>
#include <devfs/devfs.h>

/* devfsMount(): mounts the devfs file system
 * params: req - request buffer
 * params: res - response buffer
 * returns: nothing
 */

void devfsMount(SyscallHeader *req, SyscallHeader *res) {
    MountCommand *cmd = (MountCommand *) req;
    luxLogf(KPRINT_LEVEL_DEBUG, "mounting devfs at %s\n", cmd->target);

    memcpy(res, req, req->header.length);
    res->header.response = 1;
    res->header.status = 0;     // success
    luxSendDependency(res);
}