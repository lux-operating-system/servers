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
#include <termios.h>

/* ptyOpen(): handles open() syscalls for a pseudo-terminal
 * params: opencmd - open command message
 * returns: nothing, response relayed back to driver
 */

void ptyOpen(OpenCommand *opencmd) {
    if(!strcmp(opencmd->path, "/ptmx")) return ptyOpenPrimary(opencmd);
    else if(!memcmp(opencmd->path, "/pts", 4)) return ptyOpenSecondary(opencmd);

    // no such file
    opencmd->header.header.length = sizeof(OpenCommand);
    opencmd->header.header.status = -ENOENT;
    opencmd->header.header.response = 1;
    luxSendKernel(opencmd);
}

/* ptyOpenPrimary(): handles open() syscalls for the primary multiplexer
 * params: opencmd - open command message
 * returns: nothing, response relayed back to driver
 */

void ptyOpenPrimary(OpenCommand *opencmd) {
    // create a new secondary terminal for this
    char secondary[9];
    int secondaryID = -1;

    for(int i = 0; i < MAX_PTYS; i++) {
        if(!ptys[i].valid) {
            secondaryID = i;
            break;
        }
    }

    if(secondaryID < 0) {
        // no free terminals
        opencmd->header.header.length = sizeof(OpenCommand);
        opencmd->header.header.status = -ENOENT;
        opencmd->header.header.response = 1;
        luxSendKernel(opencmd);
        return;
    }

    strcpy(secondary, "/pts");
    itoa(secondaryID, &secondary[4], DECIMAL);

    ptys[secondaryID].valid = 1;
    ptys[secondaryID].index = secondaryID;
    ptys[secondaryID].openCount = 1;    // primary
    ptys[secondaryID].primary = NULL;
    ptys[secondaryID].secondary = NULL;
    ptys[secondaryID].primarySize = 0;
    ptys[secondaryID].secondarySize = 0;
    ptys[secondaryID].primaryDataSize = 0;
    ptys[secondaryID].secondaryDataSize = 0;
    ptys[secondaryID].locked = 1;

    /* reset default terminal state */
    ptys[secondaryID].termios.c_iflag = DEFAULT_IFLAG;
    ptys[secondaryID].termios.c_oflag = DEFAULT_OFLAG;
    ptys[secondaryID].termios.c_cflag = DEFAULT_CFLAG;
    ptys[secondaryID].termios.c_lflag = DEFAULT_LFLAG;
    ptys[secondaryID].termios.c_cc[VEOF] = PTY_EOF;
    ptys[secondaryID].termios.c_cc[VEOL] = PTY_EOL;
    ptys[secondaryID].termios.c_cc[VERASE] = PTY_ERASE;
    ptys[secondaryID].termios.c_cc[VINTR] = PTY_INTR;
    ptys[secondaryID].termios.c_cc[VKILL] = PTY_KILL;
    ptys[secondaryID].termios.c_cc[VMIN] = PTY_MIN;
    ptys[secondaryID].termios.c_cc[VQUIT] = PTY_QUIT;
    ptys[secondaryID].termios.c_cc[VSTART] = PTY_START;
    ptys[secondaryID].termios.c_cc[VSTOP] = PTY_STOP;
    ptys[secondaryID].termios.c_cc[VSUSP] = PTY_SUSP;
    ptys[secondaryID].termios.c_cc[VTIME] = PTY_TIME;

    ptys[secondaryID].ws.ws_col = DEFAULT_WIDTH;
    ptys[secondaryID].ws.ws_row = DEFAULT_HEIGHT;

    ptyCount++;

    // create the secondary under /dev
    // the default mode of the secondary is that of the primary; it is owned by
    // root:root with permissions rw-rw-rw-

    DevfsRegisterCommand regcmd;
    memset(&regcmd, 0, sizeof(DevfsRegisterCommand));
    regcmd.header.command = COMMAND_DEVFS_REGISTER;
    regcmd.header.length = sizeof(DevfsRegisterCommand);
    strcpy(regcmd.path, secondary);
    strcpy(regcmd.server, "lux:///dspty");  // server name prefixed with "lux:///ds"

    regcmd.status.st_mode = (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH | S_IFCHR);
    regcmd.status.st_size = 4096;
    regcmd.handleOpen = 1;

    luxSendDependency(&regcmd);

    ssize_t rs = luxRecvDependency(&regcmd, regcmd.header.length, true, false);
    if(rs < sizeof(DevfsRegisterCommand) || regcmd.header.status
    || regcmd.header.command != COMMAND_DEVFS_REGISTER) {
        luxLogf(KPRINT_LEVEL_ERROR, "failed to register pty device, error code = %d\n", regcmd.header.status);
        opencmd->header.header.length = sizeof(OpenCommand);
        opencmd->header.header.response = 1;
        opencmd->header.header.status = -EIO;
        luxSendKernel(opencmd);
        return;
    }

    // and assign the ID to the primary's file descriptor because no primary file
    // exists on the file system
    opencmd->header.header.length = sizeof(OpenCommand);
    opencmd->header.header.response = 1;
    opencmd->header.header.status = 0;  // success
    opencmd->id = (uint64_t) secondaryID;
    opencmd->charDev = 1;

    luxSendKernel(opencmd);
}

/* ptyOpenSecondary(): handles open() syscalls for secondary terminals
 * params: opencmd - open command message
 * returns: nothing, response relayed back to driver
 */

void ptyOpenSecondary(OpenCommand *opencmd) {
    opencmd->header.header.response = 1;
    opencmd->header.header.length = sizeof(OpenCommand);
    opencmd->header.header.status = 0;

    int secondaryID = atoi(&opencmd->path[8]);
    if(secondaryID < 0 || secondaryID > MAX_PTYS || !ptys[secondaryID].valid)
        opencmd->header.header.status = -ENOENT;
    else if(ptys[secondaryID].locked)
        opencmd->header.header.status = -EIO;
    else
        opencmd->id = secondaryID;
    
    if(!opencmd->header.header.status)
        opencmd->charDev = 1;
    luxSendKernel(opencmd);
}