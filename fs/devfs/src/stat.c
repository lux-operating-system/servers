/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * devfs: Microkernel server implementing the /dev file system
 */

#include <string.h>
#include <liblux/liblux.h>
#include <vfs.h>
#include <devfs/devfs.h>

/* devfsStat(): returns the file status of a file on the /dev file system
 * params: req - request buffer
 * params: res - response buffer
 * returns: nothing
 */

void devfsStat(SyscallHeader *req, SyscallHeader *res) {
    StatCommand *cmd = (StatCommand *) req;
    luxLogf(KPRINT_LEVEL_DEBUG, "stat for %s\n", cmd->path);
}
