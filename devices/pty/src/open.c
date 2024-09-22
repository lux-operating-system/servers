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

/* ptyOpen(): handles open() syscalls for a pseudo-terminal
 * params: opencmd - open command message
 * returns: nothing, response relayed back to driver
 */

void ptyOpen(OpenCommand *opencmd) {
    if(!strcmp(opencmd->path, "/ptmx")) return ptyOpenMaster(opencmd);
    else if(!memcmp(opencmd->path, "/pts", 4)) return ptyOpenSlave(opencmd);

    // no such file
    opencmd->header.header.length = sizeof(OpenCommand);
    opencmd->header.header.status = -ENOENT;
    opencmd->header.header.response = 1;
    luxSendDependency(opencmd);
}

/* ptyOpenMaster(): handles open() syscalls for the master multiplexer
 * params: opencmd - open command message
 * returns: nothing, response relayed back to driver
 */

void ptyOpenMaster(OpenCommand *opencmd) {
    // create a new slave terminal for this
    char slave[9];
    int slaveID = -1;

    for(int i = 0; i < MAX_PTYS; i++) {
        if(!ptys[i].valid) {
            slaveID = i;
            break;
        }
    }

    if(slaveID < 0) {
        // no free terminals
        opencmd->header.header.length = sizeof(OpenCommand);
        opencmd->header.header.status = -ENOENT;
        opencmd->header.header.response = 1;
        luxSendDependency(opencmd);
        return;
    }

    strcpy(slave, "/pts");
    itoa(slaveID, &slave[4], DECIMAL);

    ptys[slaveID].valid = 1;
    ptys[slaveID].index = slaveID;
    ptys[slaveID].openCount = 1;    // master

    // create the slave under /dev
    // the default mode of the slave is that of the master; it is owned by
    // root:root with permissions rw-rw-rw-

    DevfsRegisterCommand regcmd;
    memset(&regcmd, 0, sizeof(DevfsRegisterCommand));
    regcmd.header.command = COMMAND_DEVFS_REGISTER;
    regcmd.header.length = sizeof(DevfsRegisterCommand);
    strcpy(regcmd.path, slave);
    strcpy(regcmd.server, "lux:///dspty");  // server name prefixed with "lux:///ds"

    regcmd.status.st_mode = (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH | S_IFCHR);
    regcmd.status.st_size = 4096;

    luxSendDependency(&regcmd);

    // and assign the ID to the master's file descriptor because no master file
    // exists on the file system
    opencmd->header.header.length = sizeof(OpenCommand);
    opencmd->header.header.response = 1;
    opencmd->header.header.status = 0;  // success
    opencmd->id = (uint64_t) slaveID;

    luxSendDependency(opencmd);
}

/* ptyOpenSlave(): handles open() syscalls for slave terminals
 * params: opencmd - open command message
 * returns: nothing, response relayed back to driver
 */

void ptyOpenSlave(OpenCommand *opencmd) {
    
}