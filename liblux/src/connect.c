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
#include <stdlib.h>
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

    for(int i = 0; i < 16; i++) sched_yield();

    return 0;
}

/* luxInitLumen(): initializes liblux for lumen
 * params: none
 * returns: 0 on success
 */

int luxInitLumen() {
    server = "lumen";
    int status = luxConnectKernel();
    for(int i = 0; i < 32; i++) sched_yield();
    return status;
}

/* luxConnectKernel(): connects to the kernel socket
 * params: none
 * returns: 0 on success
 */

int luxConnectKernel() {
    if(kernelsd >= 0) return 0;

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

    int sd = socket(AF_UNIX, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if(sd < 0) return -1;

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
    if(lumensd >= 0) return 0;

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SERVER_LUMEN_PATH);

    int sd = socket(AF_UNIX, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if(sd < 0) return -1;

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
    if(depsd >= 0) return 0;

    // dependency address
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, "lux:///");
    strcpy(&addr.sun_path[7], name);

    // self address prefixed with lux:///ds
    struct sockaddr_un local;
    memset(&local, 0, sizeof(struct sockaddr_un));
    local.sun_family = AF_UNIX;
    strcpy(local.sun_path, "lux:///ds");
    strcpy(&local.sun_path[9], server);

    int sd = socket(AF_UNIX, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if(sd < 0) return -1;

    int status = bind(sd, (const struct sockaddr *) &local, sizeof(struct sockaddr_un));
    if(status) return -1;

    status = connect(sd, (const struct sockaddr *) &addr, sizeof(struct sockaddr_un));
    if(status) return -1;

    //luxLogf(KPRINT_LEVEL_DEBUG, "connected to '%s' at socket %d\n", addr.sun_path, sd);

    depsd = sd;
    if(!self) self = getpid();

    for(int i = 0; i < 16; i++) sched_yield();
    return 0;
}

/* luxSendKernel(): sends a message to the kernel
 * params: msg - message header
 * returns: number of bytes sent, zero or negative on fail
 */

ssize_t luxSendKernel(void *msg) {
    MessageHeader *header = (MessageHeader *) msg;
    if(!header->length || kernelsd < 0) return 0;

    if(!header->response) header->requester = self;
    return send(kernelsd, msg, header->length, 0);
}

/* luxRecvKernel(): receives a message from the kernel
 * params: buffer - buffer to store message in
 * params: len - maximum length of buffer
 * params: block - whether to block the thread
 * params: peek - whether to peek
 * returns: number of bytes read, zero or negative on fail
 */

ssize_t luxRecvKernel(void *buffer, size_t len, bool block, bool peek) {
    if(!len || !buffer) return 0;

    ssize_t size;
    do {
        size = recv(kernelsd, buffer, len, peek ? MSG_PEEK : 0);
        if(size > 0 && size <= len) {
            return size;
        } else if(size < 0) {
            if((errno != EAGAIN) && (errno != EWOULDBLOCK)) return -1;
        }
    } while(block && size <= 0);

    return 0;
}

/* luxRecvLumen(): receives a message from lumen
 * params: buffer - buffer to store message in
 * params: len - maximum length of buffer
 * params: block - whether to block the thread
 * params: peek - whether to peek
 * returns: number of bytes read, zero or negative on fail
 */

ssize_t luxRecvLumen(void *buffer, size_t len, bool block, bool peek) {
    if(!len || !buffer) return 0;

    ssize_t size;
    do {
        size = recv(lumensd, buffer, len, peek ? MSG_PEEK : 0);
        if(size > 0 && size <= len) {
            return size;
        } else if(size < 0) {
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
    if(!header->length || lumensd < 0) return 0;

    return send(lumensd, msg, header->length, 0);
}

/* luxRecvDependency(): receives a message from a dependency
 * params: buffer - buffer to store message in
 * params: len - maximum length of buffer
 * params: block - whether to block the thread
 * params: peek - whether to peek
 * returns: number of bytes read, zero or negative on fail
 */

ssize_t luxRecvDependency(void *buffer, size_t len, bool block, bool peek) {
    if(!len || !buffer) return 0;

    ssize_t size;
    do {
        size = recv(depsd, buffer, len, peek ? MSG_PEEK : 0);
        if(size > 0 && size <= len) {
            return size;
        } else if(size < 0) {
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
    if(!header->length || depsd < 0) return 0;

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
 * params: peek - whether to peek
 * returns: number of bytes read, zero or negative on fail
 */

ssize_t luxRecv(int sd, void *buffer, size_t len, bool block, bool peek) {
    if(!len || !buffer) return 0;

    ssize_t size;
    do {
        size = recv(sd, buffer, len, peek ? MSG_PEEK : 0);
        if(size > 0 && size <= len) {
            return size;
        } else if(size < 0) {
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

/* luxReady(): notifies lumen that the server has completed startup
 * params: none
 * returns: zero
 */

int luxReady() {
    MessageHeader msg;
    memset(&msg, 0, sizeof(MessageHeader));

    msg.command = COMMAND_LUMEN_READY;
    send(lumensd, &msg, sizeof(MessageHeader), 0);

    return 0;
}

static int lastRecv = 0;

static ssize_t luxRecvDK(void **buffer) {
    lastRecv = 1;
    SyscallHeader *hdr = (SyscallHeader *) *buffer;
    ssize_t s = luxRecvDependency(*buffer, SERVER_MAX_SIZE, false, true);
    if(s > 0 && s <= SERVER_MAX_SIZE) {
        if(hdr->header.length > SERVER_MAX_SIZE) {
            void *newptr = realloc(*buffer, hdr->header.length);
            hdr = newptr;
            if(!newptr) return 0;

            *buffer = newptr;
        }

        s = luxRecvDependency(*buffer, hdr->header.length, false, false);
        if(s != hdr->header.length) return 0;
        lastRecv = 1;
        return s;
    }

    s = luxRecvKernel(*buffer, SERVER_MAX_SIZE, false, true);
    if(s > 0 && s <= SERVER_MAX_SIZE) {
        if(hdr->header.length > SERVER_MAX_SIZE) {
            void *newptr = realloc(*buffer, hdr->header.length);
            hdr = newptr;
            if(!newptr) return 0;

            *buffer = newptr;
        }

        s = luxRecvKernel(*buffer, hdr->header.length, false, false);
        if(s != hdr->header.length) return 0;
        lastRecv = 0;
        return s;
    }

    return 0;
}

static ssize_t luxRecvKD(void **buffer) {
    lastRecv = 0;
    SyscallHeader *hdr = (SyscallHeader *) *buffer;
    ssize_t s = luxRecvKernel(*buffer, SERVER_MAX_SIZE, false, true);
    if(s > 0 && s <= SERVER_MAX_SIZE) {
        if(hdr->header.length > SERVER_MAX_SIZE) {
            void *newptr = realloc(*buffer, hdr->header.length);
            hdr = newptr;
            if(!newptr) return 0;

            *buffer = newptr;
        }

        s = luxRecvKernel(*buffer, hdr->header.length, false, false);
        if(s != hdr->header.length) return 0;
        return s;
    }

    s = luxRecvDependency(*buffer, SERVER_MAX_SIZE, false, true);
    if(s > 0 && s <= SERVER_MAX_SIZE) {
        if(hdr->header.length > SERVER_MAX_SIZE) {
            void *newptr = realloc(*buffer, hdr->header.length);
            hdr = newptr;
            if(!newptr) return 0;

            *buffer = newptr;
        }

        s = luxRecvDependency(*buffer, hdr->header.length, false, false);
        if(s != hdr->header.length) return 0;
        return s;
    }

    return 0;
}

/* luxRecvCommand(): receives a command from a server dependency or the kernel
 * params: buffer - pointer to buffer pointer
 * returns: number of bytes read
 */

ssize_t luxRecvCommand(void **buffer) {
    ssize_t s;
    if(lastRecv) {
        s = luxRecvKD(buffer);
        if(!s) s = luxRecvDK(buffer);
        return s;
    }

    s = luxRecvDK(buffer);
    if(!s) s = luxRecvKD(buffer);
    return s;
}