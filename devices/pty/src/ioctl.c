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
        strcpy(chstat.path, "/dev/pts");
        ltoa(cmd->id, &chstat.path[8], DECIMAL);
        chstat.status.st_mode = (S_IRUSR | S_IWUSR | S_IWGRP | S_IFCHR);
        chstat.status.st_size = 4096;
        chstat.status.st_uid = cmd->header.header.requester;
        chstat.status.st_gid = 0;   // TODO: the group of the slave pty should not be root

        luxSendDependency(&chstat);

        cmd->header.header.status = 0;
        break;
    
    case PTY_UNLOCK_PT:
        // unlocks the slave pty such that it can be opened
        // i'm actually not sure what this does, but for now we will enforce
        // not allowing slave ptys to be opened without being unlocked
        ptys[cmd->id].locked = 0;
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
    cmd->header.header.response = 1;
    cmd->header.header.length = sizeof(IOCTLCommand);

    int id = atoi(&cmd->path[4]);   // slave ID
    Pty *pty = &ptys[id];

    switch(cmd->opcode) {
    case PTY_TTY_NAME:
        // return the number of the pty device
        cmd->parameter = id;
        cmd->header.header.status = 0;
        break;
    
    case PTY_GET_CONTROL:
        cmd->parameter = pty->termios.c_cflag;
        cmd->header.header.status = 0;
        break;
    
    case PTY_GET_INPUT:
        cmd->parameter = pty->termios.c_iflag;
        cmd->header.header.status = 0;
        break;

    case PTY_GET_OUTPUT:
        cmd->parameter = pty->termios.c_oflag;
        cmd->header.header.status = 0;
        break;

    case PTY_GET_LOCAL:
        cmd->parameter = pty->termios.c_lflag;
        cmd->header.header.status = 0;
        break;

    case PTY_SET_CONTROL:
        pty->termios.c_cflag = cmd->parameter;
        cmd->header.header.status = 0;
        break;

    case PTY_SET_INPUT:
        pty->termios.c_iflag = cmd->parameter;
        cmd->header.header.status = 0;
        break;

    case PTY_SET_OUTPUT:
        pty->termios.c_oflag = cmd->parameter;
        cmd->header.header.status = 0;
        break;

    case PTY_SET_LOCAL:
        pty->termios.c_lflag = cmd->parameter;
        cmd->header.header.status = 0;
        break;

    default:
        if((cmd->opcode & IOCTL_IN_PARAM) || (cmd->opcode & IOCTL_OUT_PARAM))
            luxLogf(KPRINT_LEVEL_WARNING, "unimplemented slave pty %d ioctl() opcode 0x%X with input param %d\n", cmd->id, cmd->opcode, cmd->parameter);
        else
            luxLogf(KPRINT_LEVEL_WARNING, "unimplemented slave pty %d ioctl() opcode 0x%X\n", cmd->id, cmd->opcode);
        
        cmd->header.header.status = -ENOTTY;
    }

    luxSendDependency(cmd);
}