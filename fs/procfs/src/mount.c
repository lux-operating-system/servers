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
 * returns: nothing
 */

void procfsMount(MountCommand *req) {
    req->header.header.response = 1;
    req->header.header.length = sizeof(MountCommand);
    req->header.header.status = 0;      // success

    luxSendDependency(req);
}