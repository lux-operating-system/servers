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
#include <signal.h>
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
    
    case PTY_GET_WINSIZE:
        cmd->parameter = (ptys[cmd->id].ws.ws_col << 16) | ptys[cmd->id].ws.ws_row;
        cmd->header.header.status = 0;
        break;
    
    case PTY_SET_WINSIZE:
        ptys[cmd->id].ws.ws_row = cmd->parameter & 0xFFFF;
        ptys[cmd->id].ws.ws_col = (cmd->parameter >> 16) & 0xFFFF;
        cmd->header.header.status = 0;
        break;
    
    case PTY_SET_FOREGROUND:
        if(cmd->parameter <= 0) {
            cmd->header.header.status = -EINVAL;
        } else if(kill((pid_t) cmd->parameter, 0)) {
            cmd->header.header.status = -EPERM;
        } else {
            ptys[cmd->id].group = (pid_t) cmd->parameter;
            cmd->header.header.status = 0;
        }

        break;
    
    case PTY_GET_FOREGROUND:
        if(ptys[cmd->id].group <= 0) cmd->parameter = (1 << ((sizeof(pid_t) * 8) - 2));
        else cmd->parameter = ptys[cmd->id].group;

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
    
    case PTY_GET_WINSIZE:
        cmd->parameter = (pty->ws.ws_col << 16) | pty->ws.ws_row;
        cmd->header.header.status = 0;
        break;
    
    case PTY_SET_WINSIZE:
        pty->ws.ws_row = cmd->parameter & 0xFFFF;
        pty->ws.ws_col = (cmd->parameter >> 16) & 0xFFFF;
        cmd->header.header.status = 0;
        break;

    case PTY_SET_FOREGROUND:
        if(cmd->parameter <= 0) {
            cmd->header.header.status = -EINVAL;
        } else if(kill((pid_t) cmd->parameter, 0)) {
            cmd->header.header.status = -EPERM;
        } else {
            pty->group = (pid_t) cmd->parameter;
            cmd->header.header.status = 0;
        }

        break;

    case PTY_GET_FOREGROUND:
        if(pty->group <= 0) cmd->parameter = (1 << ((sizeof(pid_t) * 8) - 2));
        else cmd->parameter = pty->group;

        cmd->header.header.status = 0;
        break;
    
    case PTY_GET_NCSS1:
        cmd->parameter = pty->termios.c_cc[VEOF] & 0xFF;
        cmd->parameter |= (pty->termios.c_cc[VEOL] & 0xFF) << 8;
        cmd->parameter |= (pty->termios.c_cc[VERASE] & 0xFF) << 16;
        cmd->parameter |= (pty->termios.c_cc[VINTR] & 0xFF) << 24;
        cmd->parameter |= (pty->termios.c_cc[VKILL] & 0xFF) << 32;
        cmd->parameter |= (pty->termios.c_cc[VMIN] & 0xFF) << 40;
        cmd->parameter |= (pty->termios.c_cc[VQUIT] & 0xFF) << 48;
        cmd->parameter |= (pty->termios.c_cc[VSTART] & 0xFF) << 56;
        cmd->header.header.status = 0;
        break;
    
    case PTY_GET_NCSS2:
        cmd->parameter = pty->termios.c_cc[VSTOP] & 0xFF;
        cmd->parameter |= (pty->termios.c_cc[VSUSP] & 0xFF) << 8;
        cmd->parameter |= (pty->termios.c_cc[VTIME] & 0xFF) << 16;
        cmd->header.header.status = 0;
        break;
    
    case PTY_SET_NCSS1:
        pty->termios.c_cc[VEOF] = cmd->parameter & 0xFF;
        pty->termios.c_cc[VEOL] = (cmd->parameter >> 8) & 0xFF;
        pty->termios.c_cc[VERASE] = (cmd->parameter >> 16) & 0xFF;
        pty->termios.c_cc[VINTR] = (cmd->parameter >> 24) & 0xFF;
        pty->termios.c_cc[VKILL] = (cmd->parameter >> 32) & 0xFF;
        pty->termios.c_cc[VMIN] = (cmd->parameter >> 40) & 0xFF;
        pty->termios.c_cc[VQUIT] = (cmd->parameter >> 48) & 0xFF;
        pty->termios.c_cc[VSTART] = (cmd->parameter >> 56) & 0xFF;
        cmd->header.header.status = 0;
        break;
    
    case PTY_SET_NCSS2:
        pty->termios.c_cc[VSTOP] = cmd->parameter & 0xFF;
        pty->termios.c_cc[VSUSP] = (cmd->parameter >> 8) & 0xFF;
        pty->termios.c_cc[VTIME] = (cmd->parameter >> 16) & 0xFF;
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