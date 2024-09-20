/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * devfs: Microkernel server implementing the /dev file system
 */

#include <stdlib.h>
#include <sys/socket.h>
#include <liblux/liblux.h>
#include <liblux/devfs.h>
#include <devfs/devfs.h>

static int *connections;
static struct sockaddr *servers;
static socklen_t *addrlens;
static int count;

/* driverInit(): initializes the driver subsystem
 * params: none
 * returns: nothing
 */

void driverInit() {
    connections = calloc(MAX_DRIVERS, sizeof(int));
    servers = calloc(MAX_DRIVERS, sizeof(struct sockaddr));
    addrlens = calloc(MAX_DRIVERS, sizeof(socklen_t));

    if(!connections || !servers || !addrlens) {
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
        luxLogf(KPRINT_LEVEL_DEBUG, "connected to driver '%s' at socket %d\n", servers[count].sa_data, sd);
        count++;
    }
}