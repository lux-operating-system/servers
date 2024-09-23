/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * procfs: Microkernel server implementing the /proc file system
 */

#include <procfs/procfs.h>
#include <string.h>

/* procfsMount(): mounts the /proc file system
 * params: req - request buffer
 * params: res - response buffer
 * returns: nothing
 */

void procfsMount(MountCommand *req, MountCommand *res) {
    memcpy(res, req, sizeof(MountCommand));
    res->header.header.response = 1;
    res->header.header.length = sizeof(MountCommand);
    res->header.header.status = 0;      // success

    luxSendDependency(res);
}