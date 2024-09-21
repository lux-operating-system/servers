/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * devfs: Microkernel server implementing the /dev file system
 */

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <liblux/liblux.h>
#include <liblux/devfs.h>
#include <vfs.h>
#include <devfs/devfs.h>

DeviceFile *devices;
int deviceCount = 0;

int main(int argc, char **argv) {
    luxInit("devfs");                   // this will connect to lux and lumen
    while(luxConnectDependency("vfs")); // and to the virtual file system

    // show signs of life
    //luxLogf(KPRINT_LEVEL_DEBUG, "devfs server started with pid %d\n", luxGetSelf());

    SyscallHeader *req = calloc(1, SERVER_MAX_SIZE);
    SyscallHeader *res = calloc(1, SERVER_MAX_SIZE);

    if(!req || !res) {
        luxLog(KPRINT_LEVEL_ERROR, "unable to allocate memory for devfs messages\n");
        exit(-1);
    }

    // array of device files
    devices = calloc(MAX_DEVICES, sizeof(DeviceFile));
    if(!devices) {
        luxLog(KPRINT_LEVEL_ERROR, "unable to allocate memory for device files\n");
        exit(-1);
    }

    // create the basic devices
    struct stat chrstat;            // for character special devices
    memset(&chrstat, 0, sizeof(struct stat));
    chrstat.st_mode = DEVFS_CHR_PERMS;
    chrstat.st_size = 4096;

    createDevice("/null", nullIOHandler, &chrstat);
    createDevice("/zero", zeroIOHandler, &chrstat);
    createDevice("/random", randomIOHandler, &chrstat);
    createDevice("/urandom", randomIOHandler, &chrstat);

    // and create the socket for handling external device drivers
    driverInit();

    // notify the virtual file system that we are a file system driver
    VFSInitCommand init;
    memset(&init, 0, sizeof(VFSInitCommand));
    init.header.command = COMMAND_VFS_INIT;
    init.header.length = sizeof(VFSInitCommand);
    init.header.requester = luxGetSelf();
    strcpy(init.fsType, "devfs");
    luxSendDependency(&init);

    // and notify lumen that the startup is complete
    luxReady();

    while(1) {
        // wait for requests from the vfs
        ssize_t s = luxRecvDependency(req, SERVER_MAX_SIZE, false, true);
        if(s > 0 && s <= SERVER_MAX_SIZE) {
            if(req->header.length > SERVER_MAX_SIZE) {
                void *newptr = realloc(req, req->header.length);
                if(!newptr) {
                    luxLogf(KPRINT_LEVEL_ERROR, "failed to allocate memory for message handling\n");
                    exit(-1);
                }

                req = newptr;
            }

            luxRecvDependency(req, req->header.length, false, false);

            if(req->header.command >= 0x8000 && req->header.command <= MAX_SYSCALL_COMMAND && dispatchTable[req->header.command&0x7FFF]) {
                dispatchTable[req->header.command&0x7FFF](req, res);
            } else {
                luxLogf(KPRINT_LEVEL_WARNING, "unimplemented command 0x%X for pid %d\n", req->header.command, req->header.requester);
            }
        }

        driverHandle();
    }
}