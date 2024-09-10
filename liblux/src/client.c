/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * liblux: Library abstracting kernel-server communication protocols
 */

/* Client Functions to Request from the Kernel */

#include <liblux/liblux.h>

/* luxRequestFramebuffer(): requests framebuffer access
 * params: buffer - buffer to store the response in
 * returns: 0 on success
 */

int luxRequestFramebuffer(FramebufferResponse *response) {
    // construct the request
    MessageHeader request;
    request.command = COMMAND_FRAMEBUFFER;
    request.length = sizeof(MessageHeader);
    request.response = 0;

    if(luxSendKernel(&request) != request.length) return -1;

    return !(luxRecvKernel(response, sizeof(FramebufferResponse), true) == sizeof(FramebufferResponse));
}
