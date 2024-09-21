/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * kbd: Abstraction for keyboard devices under /dev/kbd
 */

#include <string.h>
#include <sys/stat.h>
#include <liblux/liblux.h>
#include <liblux/devfs.h>

int main() {
    luxInit("kbd");
    while(luxConnectDependency("devfs"));

    // create a keyboard device under /dev
    struct stat status;
    memset(&status, 0, sizeof(struct stat));
    status.st_size = 4096;

    // character device, r--r--r--
    status.st_mode = (S_IRUSR | S_IRGRP | S_IROTH | S_IFCHR);

    DevfsRegisterCommand regcmd;
    memset(&regcmd, 0, sizeof(DevfsRegisterCommand));
    regcmd.header.command = COMMAND_DEVFS_REGISTER;
    regcmd.header.length = sizeof(DevfsRegisterCommand);
    strcpy(regcmd.path, "/kbd");
    strcpy(regcmd.server, "lux:///dskbd");  // server name prefixed with "lux:///ds"
    memcpy(&regcmd.status, &status, sizeof(struct stat));
    luxSendDependency(&regcmd);

    // notify lumen that startup is complete
    luxReady();
    while(1);
}