/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * vfs: Microkernel server implementing a virtual file system
 */

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <liblux/liblux.h>
#include <vfs.h>
#include <vfs/vfs.h>

FileSystemServers *servers;
int serverCount = 0;

int main(int argc, char **argv) {
    luxInit("vfs");     // this will connect to lux and lumen

    // show signs of life
    luxLogf(KPRINT_LEVEL_DEBUG, "virtual file system server started with pid %d\n", getpid());

    SyscallHeader *req = calloc(1, SERVER_MAX_SIZE);
    servers = calloc(MAX_FILE_SYSTEMS, sizeof(FileSystemServers));
    mps = calloc(MAX_MOUNTPOINTS, sizeof(Mountpoint));

    if(!req || !servers || !mps) {
        luxLog(KPRINT_LEVEL_ERROR, "unable to allocate memory for vfs\n");
        return -1;
    }

    ssize_t s;
    int sd;
    while(1) {
        // accept incoming client connections
        sd = luxAccept();
        if(sd > 0) {
            // append to the list
            servers[serverCount].socket = sd;
            serverCount++;
        }

        // and handle messages from dependent servers
        for(int i = 0; serverCount && i < serverCount; i++) {
            s = luxRecv(servers[i].socket, req, SERVER_MAX_SIZE, false);
            if(s > 0 && s <= SERVER_MAX_SIZE) {
                if(req->header.command == COMMAND_VFS_INIT) {   // special command
                    VFSInitCommand *init = (VFSInitCommand *)req;
                    strcpy(servers[i].type, init->fsType);
                    luxLogf(KPRINT_LEVEL_DEBUG, "loaded file system driver for '%s'\n", servers[i].type);
                } else if(req->header.command >= 0x8000 && req->header.command <= MAX_SYSCALL_COMMAND) {
                    if(req->header.command == COMMAND_MOUNT) registerMountpoint((MountCommand *)req);
                    luxSendLumen(req);  // relay responses to lumen
                } else {
                    luxLogf(KPRINT_LEVEL_WARNING, "unimplemented response to command 0x%X from file system driver for '%s'\n", req->header.command, servers[i].type);
                }
            }
        }

        // and wait for incoming syscall requests
        s = luxRecvLumen(req, SERVER_MAX_SIZE, false);
        if(s > 0 && s <= SERVER_MAX_SIZE) {
            // dispatch syscall request from the kernel
            if(req->header.command >= 0x8000 && req->header.command <= MAX_SYSCALL_COMMAND && vfsDispatchTable[req->header.command&0x7FFF]) {
                vfsDispatchTable[req->header.command&0x7FFF](req);
            } else {
                luxLogf(KPRINT_LEVEL_WARNING, "unimplemented syscall request 0x%X for pid %d\n", req->header.command, req->header.requester);
            }
        }
    }
}