/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * lfb: Abstraction for linear frame buffers under /dev/lfb
 */

#include <liblux/liblux.h>
#include <liblux/devfs.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>

static FramebufferResponse fb;
static void *buffer;

int main() {
    luxInit("lfb");
    while(luxConnectDependency("devfs"));   // depend on /dev

    // request the frame buffer from the kernel
    if(luxRequestFramebuffer(&fb)) {
        luxLogf(KPRINT_LEVEL_ERROR, "failed to acquire from kernel\n");
        return -1;
    }

    luxLogf(KPRINT_LEVEL_DEBUG, "screen resolution is %dx%d (%d bpp)\n", fb.w, fb.h, fb.bpp);

    // create a character device on /dev for the frame buffer
    DevfsRegisterCommand *regcmd = calloc(1, sizeof(DevfsRegisterCommand));
    struct stat *status = calloc(1, sizeof(struct stat));
    buffer = malloc(fb.w * fb.h * fb.bpp / 8);  // back buffer
    if(!regcmd || !status || !buffer) {
        luxLogf(KPRINT_LEVEL_ERROR, "failed to allocate memory for frame buffer device\n");
        return -1;
    }

    // character device with permissions rw-rw-r--
    status->st_mode = (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IFCHR);
    status->st_size = fb.w * fb.h * fb.bpp / 8;

    // register the device
    regcmd->header.command = COMMAND_DEVFS_REGISTER;
    regcmd->header.length = sizeof(DevfsRegisterCommand);
    strcpy(regcmd->path, "/lfb0");
    strcpy(regcmd->server, "lux:///dslfb");  // server name prefixed with "lux:///ds"
    memcpy(&regcmd->status, status, sizeof(struct stat));
    luxSendDependency(regcmd);

    free(status);
    free(regcmd);

    // notify lumen that startup is complete
    luxReady();
    for(;;);
}