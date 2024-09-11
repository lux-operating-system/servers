/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * liblux: Library abstracting kernel-server communication protocols
 */

/* Client Functions to Request from the Kernel */

#include <liblux/liblux.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static char formatter[1024];

/* luxLog(): prints a log message
 * params: level - log severity level
 * params: msg - message to print
 * returns: nothing */

void luxLog(int level, const char *msg) {
    LogCommand *log = malloc(sizeof(LogCommand) + strlen(msg) + 1);
    if(!log) return;

    log->header.command = COMMAND_LOG;
    log->header.length = sizeof(LogCommand) + strlen(msg) + 1;
    log->header.response = 0;
    log->level = level;
    strcpy(log->server, luxGetName());
    strcpy(log->message, msg);

    luxSendKernel(log);
    free(log);
}

/* luxLogf(): prints a formatted log message
 * params: level - log severity level
 * params: f - formatter string
 * returns: nothing
 */

void luxLogf(int level, const char *f, ...) {
    va_list args;
    va_start(args, f);
    vsprintf(formatter, f, args);
    va_end(args);
    luxLog(level, formatter);
}

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
