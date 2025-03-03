/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * devfs: Microkernel server implementing the /dev file system
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <liblux/liblux.h>
#include <liblux/devfs.h>
#include <devfs/devfs.h>

static int *connections;
static struct sockaddr *servers;
static socklen_t *addrlens;
static int count;
static void *in, *out;
static void (*driverDispatch[])(int, MessageHeader *, MessageHeader *);

/* driverInit(): initializes the driver subsystem
 * params: none
 * returns: nothing
 */

void driverInit() {
    connections = calloc(MAX_DRIVERS, sizeof(int));
    servers = calloc(MAX_DRIVERS, sizeof(struct sockaddr));
    addrlens = calloc(MAX_DRIVERS, sizeof(socklen_t));
    in = calloc(1, SERVER_MAX_SIZE);
    out = calloc(1, SERVER_MAX_SIZE);

    if(!connections || !servers || !addrlens || !in || !out) {
        luxLogf(KPRINT_LEVEL_ERROR, "failed to allocate memory for driver subsystem\n");
        exit(-1);
    }

    count = 0;
}

/* driverHandle(): handles incoming requests from drivers
 * params: none
 * returns: nothing
 */

void driverHandle() {
    // accept incoming connections
    int actions = 0;
    addrlens[count] = sizeof(struct sockaddr);
    int sd = luxAcceptAddr(&servers[count], &addrlens[count]);
    if(sd > 0) {
        actions++;
        connections[count] = sd;
        luxLogf(KPRINT_LEVEL_DEBUG, "connected to driver '%s' at socket %d\n", &servers[count].sa_data[9], sd);
        count++;
    }

    if(!count) {
        sched_yield();
        return;
    }

    // and receive requests from dependent servers
    for(int i = 0; i < count; i++) {
        ssize_t s = luxRecv(connections[i], in, SERVER_MAX_SIZE, false, true);
        if((s > 0) && (s <= SERVER_MAX_SIZE)) {
            MessageHeader *hdr = (MessageHeader *) in;

            if(hdr->length > SERVER_MAX_SIZE) {
                void *newptr = realloc(in, hdr->length);
                if(!newptr) {
                    luxLogf(KPRINT_LEVEL_ERROR, "failed to allocate memory for message handling\n");
                    exit(-1);
                }

                hdr = newptr;
                in = newptr;
            }

            luxRecv(connections[i], in, hdr->length, false, false);
            actions++;

            if(hdr->command == COMMAND_READ || hdr->command == COMMAND_WRITE ||
                hdr->command == COMMAND_OPEN || hdr->command == COMMAND_IOCTL) {
                // driver is responding to an open/read/write request; simply relay it back to the kernel
                luxSendKernel(hdr);
            } else if((hdr->command >= COMMAND_MIN_DEVFS) && (hdr->command <= COMMAND_MAX_DEVFS) && driverDispatch[hdr->command & (~COMMAND_MIN_DEVFS)]) {
                driverDispatch[hdr->command & (~COMMAND_MIN_DEVFS)](connections[i], (MessageHeader *) in, (MessageHeader *)out);
            } else {
                luxLogf(KPRINT_LEVEL_WARNING, "undefined request from driver '%s': 0x%04X, dropping message...\n",
                &servers[i].sa_data[9], hdr->command);
            }
        }
    }

    if(!actions) sched_yield();
}

/* driverRegister(): registers an external device on the /dev file system
 * params: sd - inbound socket descriptor
 * params: cmd - inbound command
 * params: buf - outbound response buffer
 * returns: nothing
 */

void driverRegister(int sd, MessageHeader *cmd, MessageHeader *buf) {
    DevfsRegisterCommand *regcmd = (DevfsRegisterCommand *) cmd;
    if(createDevice(regcmd->path, NULL, &regcmd->status)) {
        luxLogf(KPRINT_LEVEL_ERROR, "failed to register device '/dev%s' for server '%s\n", regcmd->path, &regcmd->server[9]);
        return;
    }

    DeviceFile *dev = findDevice(regcmd->path);
    dev->server = malloc(256);
    if(!dev->server) return;

    dev->external = 1;
    dev->socket = sd;
    strcpy(dev->server, regcmd->server);
    dev->handleOpen = regcmd->handleOpen;

    //luxLogf(KPRINT_LEVEL_DEBUG, "device '/dev%s' handled by server '%s' on socket %d\n", dev->name, &dev->server[9], dev->socket);

    regcmd->header.response = 1;
    regcmd->header.status = 0;
    luxSend(sd, regcmd);
}

/* driverChstat(): change the status of a device managaed by an external driver
 * params: sd - inbound socket descriptor
 * params: cmd - inbound command
 * params: buf - outbound response buffer
 */

void driverChstat(int sd, MessageHeader *cmd, MessageHeader *buf) {
    DevfsChstatCommand *chstatcmd = (DevfsChstatCommand *) cmd;

    DeviceFile *dev = findDevice(chstatcmd->path);
    if(!dev || !dev->external) return;

    memcpy(&dev->status, &chstatcmd->status, sizeof(struct stat));
}

/* driverRead(): reads from a device file handled by an external driver
 * params: cmd - read command message
 * params: dev - device file structure
 * returns: nothing
 */

void driverRead(RWCommand *cmd, DeviceFile *dev) {
    luxSend(dev->socket, cmd);
}

/* driverWrite(): writes to a device file handled by an external driver
 * params: cmd - write command message
 * params: dev - device file structure
 * returns: nothing
 */

void driverWrite(RWCommand *cmd, DeviceFile *dev) {
    luxSend(dev->socket, cmd);
}

/* dispatch table */

static void (*driverDispatch[])(int, MessageHeader *, MessageHeader *) = {
    driverRegister,         // 0 - register a /dev device
    NULL,                   // 1 - unregister a /dev device
    NULL,                   // 2 - status
    driverChstat,           // 3 - change status of /dev device
};
