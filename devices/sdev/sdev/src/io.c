/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * sdev: Abstraction for storage devices under /dev/sdX
 */

#include <liblux/liblux.h>
#include <liblux/sdev.h>
#include <sdev/sdev.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* sdevRead(): reads from a storage device
 * params: cmd - read command message
 * returns: nothing, request relayed to storage device driver
 */

void sdevRead(RWCommand *cmd) {
    int i = atoi(&cmd->path[3]);
    StorageDevice *dev = findDevice(i);
    if(!dev) {
        // no such entry somehow - possible hotpluggable device removed
        cmd->header.header.response = 1;
        cmd->header.header.length = sizeof(RWCommand);
        cmd->header.header.status = -ENODEV;
        cmd->length = 0;
        luxSendDependency(cmd);
        return;
    }

    // relay the request to the appropriate device driver
    SDevRWCommand rcmd;
    memset(&rcmd, 0, sizeof(SDevRWCommand));
    rcmd.header.command = COMMAND_SDEV_READ;
    rcmd.header.length = sizeof(SDevRWCommand);
    rcmd.syscall = cmd->id;
    rcmd.start = cmd->position;
    rcmd.count = cmd->length;
    rcmd.device = dev->deviceID;
    luxSend(dev->sd, &rcmd);
}