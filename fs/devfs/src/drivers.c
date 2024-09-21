/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * devfs: Microkernel server implementing the /dev file system
 */

#include <stdlib.h>
#include <string.h>
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
    addrlens[count] = sizeof(struct sockaddr);
    int sd = luxAcceptAddr(&servers[count], &addrlens[count]);
    if(sd > 0) {
        connections[count] = sd;
        luxLogf(KPRINT_LEVEL_DEBUG, "connected to driver '%s' at socket %d\n", &servers[count].sa_data[9], sd);
        count++;
    }

    if(!count) return;

    // and receive requests from dependent servers
    for(int i = 0; i < count; i++) {
        ssize_t s = luxRecv(connections[i], in, SERVER_MAX_SIZE, false);
        if((s > 0) & (s <= SERVER_MAX_SIZE)) {
            MessageHeader *hdr = (MessageHeader *) in;
            if((hdr->command >= COMMAND_MIN_DEVFS) && (hdr->command <= COMMAND_MAX_DEVFS) && driverDispatch[hdr->command & (~COMMAND_MIN_DEVFS)]) {
                driverDispatch[hdr->command & (~COMMAND_MIN_DEVFS)](connections[i], (MessageHeader *) in, (MessageHeader *)out);
            } else {
                luxLogf(KPRINT_LEVEL_WARNING, "undefined request from driver '%s': 0x%04X, dropping message...\n",
                &servers[i].sa_data[9], hdr->command);
            }
        }
    }
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
        luxLogf(KPRINT_LEVEL_ERROR, "failed to register device '/dev%s' for server '%s\n", regcmd->path, &dev->server[9]);
        return;
    }

    DeviceFile *dev = findDevice(regcmd->path);
    dev->external = 1;
    dev->socket = sd;
    strcpy(dev->server, regcmd->server);

    luxLogf(KPRINT_LEVEL_DEBUG, "device '/dev%s' handled by server '%s' on socket %d\n", dev->name, &dev->server[9], dev->socket);
}

/* dispatch table */

static void (*driverDispatch[])(int, MessageHeader *, MessageHeader *) = {
    driverRegister,         // 0 - register a /dev device
    NULL,                   // 1 - unregister a /dev device
    NULL,                   // 2 - status
};
