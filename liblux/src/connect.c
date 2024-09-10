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

static int kernelsd = -1, lumensd = -1;
static pid_t self = 0;

/* luxConnectKernel(): connects to the kernel socket
 * params: none
 * returns: 0 on success
 */

int luxConnectKernel() {
    if(kernelsd > 0) return 0;

    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SERVER_KERNEL_PATH);

    int sd = socket(AF_UNIX, SOCK_DGRAM | SOCK_NONBLOCK, 0);
    if(sd <= 0) return -1;

    int status = connect(sd, (const struct sockaddr *) &addr, sizeof(struct sockaddr_un));
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
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SERVER_LUMEN_PATH);

    int sd = socket(AF_UNIX, SOCK_DGRAM | SOCK_NONBLOCK, 0);
    if(sd <= 0) return -1;

    int status = connect(sd, (const struct sockaddr *) &addr, sizeof(struct sockaddr_un));
    if(status) return -1;

    lumensd = sd;
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

    header->requester = self;
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
        if(size > 0) {
            return size;
        } else if(size == -1) {
            if((!errno == EAGAIN) && (!errno == EWOULDBLOCK)) return -1;
        }
    } while(block && size <= 0);

    return 0;
}
