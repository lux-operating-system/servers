/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * pty: Microkernel server implementing Unix 98-style pseudo-terminal devices
 */

#include <liblux/liblux.h>
#include <liblux/devfs.h>
#include <pty/pty.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

/* ptyIoctl(): handles ioctl() syscalls for a pseudo-terminal
 * params: cmd - ioctl() command message
 * returns: nothing, response relayed back to driver
 */

void ptyIoctl(IOCTLCommand *cmd) {
    // verify if this is a master or slave terminal
    if(!strcmp(cmd->path, "/ptmx")) return ptyIoctlMaster(cmd);
    else if(!memcmp(cmd->path, "/pts", 4)) return ptyIoctlSlave(cmd);

    // no such file
    cmd->header.header.length = sizeof(IOCTLCommand);
    cmd->header.header.status = -ENOENT;
    cmd->header.header.response = 1;
    luxSendDependency(cmd);
}

/* ptyIoctlMaster(): handles ioctl() syscalls for a master terminal
 * params: cmd - ioctl() command message
 * returns: nothing, response relayed back to driver
 */

void ptyIoctlMaster(IOCTLCommand *cmd) {
    cmd->header.header.response = 1;
    cmd->header.header.length = sizeof(IOCTLCommand);

    switch(cmd->opcode) {
    case PTY_GET_SLAVE:
        cmd->parameter = cmd->id;
        cmd->header.header.status = 0;
        break;
    
    case PTY_GRANT_PT:
        // grantpt() changes the owner of the slave to match the UID of the
        // calling process, and changes its permissions to rw--w----
        // request this change from the devfs driver
        DevfsChstatCommand chstat;
        memset(&chstat, 0, sizeof(DevfsChstatCommand));

        chstat.header.command = COMMAND_DEVFS_CHSTAT;
        chstat.header.length = sizeof(DevfsChstatCommand);
        strcpy(chstat.path, cmd->path);
        chstat.status.st_mode = (S_IRUSR | S_IWUSR | S_IWGRP | S_IFCHR);
        chstat.status.st_size = 4096;
        chstat.status.st_uid = cmd->header.header.requester;
        chstat.status.st_gid = 0;   // TODO: the group of the slave pty should not be root

        luxSendDependency(&chstat);

        cmd->header.header.status = 0;
        break;

    default:
        if((cmd->opcode & IOCTL_IN_PARAM) || (cmd->opcode & IOCTL_OUT_PARAM))
            luxLogf(KPRINT_LEVEL_WARNING, "unimplemented master pty %d ioctl() opcode 0x%X with input param %d\n", cmd->id, cmd->opcode, cmd->parameter);
        else
            luxLogf(KPRINT_LEVEL_WARNING, "unimplemented master pty %d ioctl() opcode 0x%X\n", cmd->id, cmd->opcode);
        
        cmd->header.header.status = -ENOTTY;
        break;
    }

    luxSendDependency(cmd);
}

/* ptyIoctlSlave(): handles ioctl() syscalls for a slave terminal
 * params: cmd - ioctl() command message
 * returns: nothing, response relayed back to driver
 */

void ptyIoctlSlave(IOCTLCommand *cmd) {
    /* todo */
    cmd->header.header.response = 1;
    cmd->header.header.length = sizeof(IOCTLCommand);
    cmd->header.header.status = -ENOTTY;
    luxSendDependency(cmd);
}