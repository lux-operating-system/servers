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

static int kernelsd = -1, lumensd = -1;

/* luxConnectKernel(): connects to the kernel socket
 * params: none
 * returns: 0 on success
 */

int luxConnectKernel() {
    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SERVER_KERNEL_PATH);

    int sd = socket(AF_UNIX, SOCK_DGRAM | SOCK_NONBLOCK, 0);
    if(sd <= 0) return -1;

    int status = connect(sd, (const struct sockaddr *) &addr, sizeof(struct sockaddr_un));
    if(status) return -1;

    kernelsd = sd;
    return 0;
}

/* luxConnectLumen(): connects to lumen
 * params: none
 * returns: 0 on success
 */

int luxConnectLumen() {
    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SERVER_LUMEN_PATH);

    int sd = socket(AF_UNIX, SOCK_DGRAM | SOCK_NONBLOCK, 0);
    if(sd <= 0) return -1;

    int status = connect(sd, (const struct sockaddr *) &addr, sizeof(struct sockaddr_un));
    if(status) return -1;

    lumensd = sd;
    return 0;
}
