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

void devfsOpen(SyscallHeader *req, SyscallHeader *res) {
    OpenCommand *cmd = (OpenCommand *) req;
    memcpy(res, req, req->header.length);
    luxLogf(KPRINT_LEVEL_DEBUG, "opening file '%s'\n", cmd->path);
}