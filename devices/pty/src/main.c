/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * pty: Microkernel server implementing Unix 98-style pseudo-terminal devices
 */

/* Note to self:
 *
 * The master pseudo-terminal multiplexer is located at /dev/ptmx, and slave
 * pseudo-terminals are at /dev/ptsX. Master pseudo-terminals do not have a
 * file system representation, and they are only accessed through their file
 * descriptors. Every time an open() syscall opens /dev/ptmx, a new master-
 * slave pseudo-terminal pair is created, the file descriptor of the master is
 * returned, and the slave is created in /dev/ptsX. The master can find the
 * name of the slave using ptsname(), and the slave is deleted from the file
 * system after no more processes have an open file descriptor pointing to it.
 * 
 * After the master is created, the slave's permissions are adjusted by calling
 * grantpt() with the master file descriptor. Next, the slave is unlocked by
 * calling unlockpt() with the master file descriptor. Finally, the controlling
 * process calls ptsname() to find the slave's file name and opens it using the
 * standard open(). The opened slave file descriptor can then be set to the
 * controlling pseudo-terminal of a process using ioctl(). The input/output of
 * the slave can be read/written through the master, enabling the controlling
 * process to implement a terminal emulator.
 * 
 * https://unix.stackexchange.com/questions/405972/
 * https://unix.stackexchange.com/questions/117981/
 * https://man7.org/linux/man-pages/man7/pty.7.html
 * https://man7.org/linux/man-pages/man3/grantpt.3.html
 * https://man7.org/linux/man-pages/man3/unlockpt.3.html
 */

#include <liblux/liblux.h>
#include <liblux/devfs.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <pty/pty.h>

Pty *ptys;
int ptyCount;

int main() {
    luxInit("pty");
    while(luxConnectDependency("devfs"));   // depend on /dev

    // create the master multiplexer device, /dev/ptmx
    struct stat *status = calloc(1, sizeof(struct stat));
    DevfsRegisterCommand *regcmd = calloc(1, sizeof(DevfsRegisterCommand));
    SyscallHeader *msg = calloc(1, SERVER_MAX_SIZE);
    ptys = calloc(MAX_PTYS, sizeof(Pty));
    ptyCount = 0;

    if(!status || !regcmd || !msg || !ptys) {
        luxLogf(KPRINT_LEVEL_ERROR, "failed to allocate memory for pty server\n");
        return -1;
    }

    // character special file owned by root:root and rw-rw-rw
    // this is following the linux implementation
    status->st_mode = (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH | S_IFCHR);
    status->st_uid = 0;
    status->st_gid = 0;
    status->st_size = 4096;

    // construct the devfs register command
    regcmd->header.command = COMMAND_DEVFS_REGISTER;
    regcmd->header.length = sizeof(DevfsRegisterCommand);
    regcmd->handleOpen = 1;         // we need to handle open() here and override the vfs
    strcpy(regcmd->path, "/ptmx");
    strcpy(regcmd->server, "lux:///dspty");  // server name prefixed with "lux:///ds"
    memcpy(&regcmd->status, status, sizeof(struct stat));
    luxSendDependency(regcmd);

    free(status);
    free(regcmd);

    // notify lumen that this server is ready
    luxReady();

    for(;;) {
        ssize_t s = luxRecvDependency(msg, SERVER_MAX_SIZE, false, true);   // peek first
        if(s > 0 && s <= SERVER_MAX_SIZE) {
            if(msg->header.length > SERVER_MAX_SIZE) {
                void *newptr = realloc(msg, msg->header.length);
                if(!newptr) {
                    luxLogf(KPRINT_LEVEL_ERROR, "unable to allocate memory for message handling\n");
                    return -1;
                }

                msg = newptr;
            }

            // receive the actual message
            luxRecvDependency(msg, msg->header.length, false, false);

            switch(msg->header.command) {
            case COMMAND_OPEN: ptyOpen((OpenCommand *) msg); break;
            case COMMAND_IOCTL: ptyIoctl((IOCTLCommand *) msg); break;
            default:
                luxLogf(KPRINT_LEVEL_WARNING, "unimplemented command 0x%X, dropping message...\n", msg->header.command);
            }
        }
    }
}