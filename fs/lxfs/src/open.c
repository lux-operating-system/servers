/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * lxfs: Driver for the lxfs file system
 */

#include <liblux/liblux.h>
#include <lxfs/lxfs.h>
#include <vfs.h>
#include <string.h>
#include <stdlib.h>
#include <fnctl.h>
#include <unistd.h>
#include <errno.h>

/* lxfsOpen(): opens a opened file on an lxfs volume
 * params: ocmd - open command message
 * returns: nothing, response relayed to vfs
 */

void lxfsOpen(OpenCommand *ocmd) {
    ocmd->header.header.response = 1;
    ocmd->header.header.length = sizeof(OpenCommand);

    luxLogf(KPRINT_LEVEL_DEBUG, "opening %s on %s\n", ocmd->path, ocmd->device);

    Mountpoint *mp = findMP(ocmd->device);
    if(!mp) {
        ocmd->header.header.status = -EIO;  // device doesn't exist
        luxSendDependency(ocmd);
        return;
    }

    LXFSDirectoryEntry entry;
    if(!lxfsFind(&entry, mp, ocmd->path)) {
        ocmd->header.header.status = -ENOENT;   // file doesn't exist
        luxSendDependency(ocmd);
        return;
    }
}