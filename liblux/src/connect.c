/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * liblux: Library abstracting kernel-server communication protocols
 */

#include <liblux/liblux.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>

static int kernelsd = -1, lumensd = -1, depsd = -1;
static pid_t self = 0;
static const char *server;

/* luxInit(): initializes liblux
 * params: name - server name
 * returns: 0 on success
 */

int luxInit(const char *name) {
    if(!name) return -1;
    if(strlen(name) > 504) return -1;   // total limit is 511, minus 7 for "lux:///" prefix

    server = name;
    if(luxConnectKernel()) return -1;
    if(luxConnectLumen()) return -1;

    return 0;
}

/* luxInitLumen(): initializes liblux for lumen
 * params: none
 * returns: 0 on success
 */

int luxInitLumen() {
    server = "lumen";
    return luxConnectKernel();
}

/* luxConnectKernel(): connects to the kernel socket
 * params: none
 * returns: 0 on success
 */

int luxConnectKernel() {
    if(kernelsd > 0) return 0;

    // kernel address
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SERVER_KERNEL_PATH);

    // server address
    struct sockaddr_un local;
    memset(&local, 0, sizeof(struct sockaddr_un));
    local.sun_family = AF_UNIX;
    strcpy(local.sun_path, "lux:///ks");
    strcpy(local.sun_path+strlen(local.sun_path), server);

    int sd = socket(AF_UNIX, SOCK_DGRAM | SOCK_NONBLOCK, 0);
    if(sd <= 0) return -1;

    int status = bind(sd, (const struct sockaddr *) &local, sizeof(struct sockaddr_un));
    if(status) return -1;

    status = connect(sd, (const struct sockaddr *) &addr, sizeof(struct sockaddr_un));
    if(status) return -1;

    kernelsd = sd;
    if(!self) self = getpid();
    return 0;
}

/* luxConnectLumen(): connects to lumen
 * params: none
 * returns: 0 on success
 */

int luxConnectLumen() {
    if(lumensd > 0) return 0;

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SERVER_LUMEN_PATH);

    int sd = socket(AF_UNIX, SOCK_DGRAM | SOCK_NONBLOCK, 0);
    if(sd <= 0) return -1;

    // bind the local address so lumen knows which server this is
    struct sockaddr_un local;
    memset(&local, 0, sizeof(struct sockaddr_un));
    local.sun_family = AF_UNIX;
    strcpy(local.sun_path, "lux:///");
    strcpy(&local.sun_path[7], server);

    int status = bind(sd, (const struct sockaddr *) &local, sizeof(struct sockaddr_un));
    if(status) return -1;

    status = connect(sd, (const struct sockaddr *) &addr, sizeof(struct sockaddr_un));
    if(status) return -1;

    // and listen on the same socket
    status = listen(sd, 0);     // default backlog
    if(status) return -1;

    lumensd = sd;
    if(!self) self = getpid();
    return 0;
}

/* luxConnectDependency(): connects to a dependency server socket
 * params: name - name of the dependency
 * returns: 0 on success
 */

int luxConnectDependency(const char *name) {
    if(depsd > 0) return 0;

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, "lux:///");
    strcpy(&addr.sun_path[7], name);

    int sd = socket(AF_UNIX, SOCK_DGRAM | SOCK_NONBLOCK, 0);
    if(sd <= 0) return -1;

    int status = connect(sd, (const struct sockaddr *) &addr, sizeof(struct sockaddr_un));
    if(status) return -1;

    //luxLogf(KPRINT_LEVEL_DEBUG, "connected to '%s' at socket %d\n", addr.sun_path, sd);

    depsd = sd;
    if(!self) self = getpid();
    return 0;
}

/* luxSendKernel(): sends a message to the kernel
 * params: msg - message header
 * returns: number of bytes sent, zero or negative on fail
 */

ssize_t luxSendKernel(void *msg) {
    MessageHeader *header = (MessageHeader *) msg;
    if(!header->length || kernelsd <= 0) return 0;

    if(!header->response) header->requester = self;
    return send(kernelsd, msg, header->length, 0);
}

/* luxRecvKernel(): receives a message from the kernel
 * params: buffer - buffer to store message in
 * params: len - maximum length of buffer
 * params: block - whether to block the thread
 * returns: number of bytes read, zero or negative on fail
 */

ssize_t luxRecvKernel(void *buffer, size_t len, bool block) {
    if(!len || !buffer) return 0;

    ssize_t size;
    do {
        size = recv(kernelsd, buffer, len, 0);
        if(size > 0 && size <= len) {
            return size;
        } else if(size == -1) {
            if((errno != EAGAIN) && (errno != EWOULDBLOCK)) return -1;
        }
    } while(block && size <= 0);

    return 0;
}

/* luxRecvLumen(): receives a message from lumen
 * params: buffer - buffer to store message in
 * params: len - maximum length of buffer
 * params: block - whether to block the thread
 * returns: number of bytes read, zero or negative on fail
 */

ssize_t luxRecvLumen(void *buffer, size_t len, bool block) {
    if(!len || !buffer) return 0;

    ssize_t size;
    do {
        size = recv(lumensd, buffer, len, 0);
        if(size > 0 && size <= len) {
            return size;
        } else if(size == -1) {
            if((errno != EAGAIN) && (errno != EWOULDBLOCK)) return -1;
        }
    } while(block && size <= 0);

    return 0;
}

/* luxSendLumen(): sends a message to lumen
 * params: msg - message header
 * returns: number of bytes sent, zero or negative on fail
 */

ssize_t luxSendLumen(void *msg) {
    MessageHeader *header = (MessageHeader *) msg;
    if(!header->length || kernelsd <= 0) return 0;

    return send(lumensd, msg, header->length, 0);
}

/* luxRecvDependency(): receives a message from a dependency
 * params: buffer - buffer to store message in
 * params: len - maximum length of buffer
 * params: block - whether to block the thread
 * returns: number of bytes read, zero or negative on fail
 */

ssize_t luxRecvDependency(void *buffer, size_t len, bool block) {
    if(!len || !buffer) return 0;

    ssize_t size;
    do {
        size = recv(depsd, buffer, len, 0);
        if(size > 0 && size <= len) {
            return size;
        } else if(size == -1) {
            if((errno != EAGAIN) && (errno != EWOULDBLOCK)) return -1;
        }
    } while(block && size <= 0);

    return 0;
}

/* luxSendDependency(): sends a message to a dependency
 * params: msg - message header
 * returns: number of bytes sent, zero or negative on fail
 */

ssize_t luxSendDependency(void *msg) {
    MessageHeader *header = (MessageHeader *) msg;
    if(!header->length || depsd <= 0) return 0;

    return send(depsd, msg, header->length, 0);
}

/* luxGetSelf(): returns the current pid without a syscall
 * params: none
 * returns: process ID
 */

pid_t luxGetSelf() {
    return self;
}

/* luxGetName(): returns the name of the server
 * params: none
 * returns: pointer to name
 */

const char *luxGetName() {
    return server;
}

/* luxGetKernelSocket(): returns the kernel socket descriptor
 * params: none
 * returns: socket descriptor
 */

int luxGetKernelSocket() {
    return kernelsd;
}

/* luxAccept(): accepts a connection from a dependent server
 * params: none
 * returns: positive socket descriptor on success
 */

int luxAccept() {
    return accept(lumensd, NULL, NULL);
}

/* luxAcceptAddr(): accepts a connection from a dependent server preserving the address
 * params: addr - buffer to store the address
 * params: len - pointer to the length of the buffer at addr on input, actual size on output
 * returns: positive socket descriptor on success
 */

int luxAcceptAddr(struct sockaddr *addr, socklen_t *len) {
    return accept(lumensd, addr, len);
}

/* luxRecv(): receives a message from a dependent
 * params: sd - socket descriptor
 * params: buffer - buffer to store message in
 * params: len - maximum length of buffer
 * params: block - whether to block the thread
 * returns: number of bytes read, zero or negative on fail
 */

ssize_t luxRecv(int sd, void *buffer, size_t len, bool block) {
    if(!len || !buffer) return 0;

    ssize_t size;
    do {
        size = recv(sd, buffer, len, 0);
        if(size > 0 && size <= len) {
            return size;
        } else if(size == -1) {
            if((errno != EAGAIN) && (errno != EWOULDBLOCK)) return -1;
        }
    } while(block && size <= 0);

    return 0;
}

/* luxSend(): sends a message to a dependent
 * params: sd - socket descriptor
 * params: msg - message header
 * returns: number of bytes sent, zero or negative on fail
 */

ssize_t luxSend(int sd, void *msg) {
    MessageHeader *header = (MessageHeader *) msg;
    if(!header->length) return 0;

    return send(sd, msg, header->length, 0);
}