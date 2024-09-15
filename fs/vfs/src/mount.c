/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * vfs: Microkernel server implementing a virtual file system
 */

#include <vfs/vfs.h>
#include <liblux/liblux.h>
#include <vfs.h>
#include <string.h>

Mountpoint *mps;
int mpCount = 0;

/* registerMountpoint(): registers a mountpoint if successful
 * params: cmd - mount response message
 * returns: nothing
 */

void registerMountpoint(MountCommand *cmd) {
    if(mpCount >= MAX_MOUNTPOINTS) return;
    if(cmd->header.header.command != COMMAND_MOUNT || !cmd->header.header.response) return;
    if(cmd->header.header.status) return;

    mps[mpCount].valid = 1;
    mps[mpCount].flags = cmd->flags;
    strcpy(mps[mpCount].device, cmd->source);
    strcpy(mps[mpCount].path, cmd->target);
    strcpy(mps[mpCount].type, cmd->type);

    //luxLogf(KPRINT_LEVEL_DEBUG, "mounted '%s' at '%s'\n", mps[mpCount].type, mps[mpCount].path);

    mpCount++;
}