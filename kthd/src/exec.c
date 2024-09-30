/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * kthd: Kernel Thread Helper Daemon
 */

#include <liblux/liblux.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fnctl.h>
#include <errno.h>

void exec(ExecCommand *cmd) {
    cmd->header.header.response = 1;
    cmd->header.header.length = sizeof(ExecCommand);

    // open the program to be executed
    int fd = open(cmd->path, O_RDONLY);
    if(fd < 0) {
        cmd->header.header.status = -ENOENT;
        luxSendLumen(cmd);
        return;
    }

    // ensure the requesting process has execute permissions
    struct stat st;
    if(fstat(fd, &st)) {
        close(fd);
        cmd->header.header.status = -1*errno;
        luxSendLumen(cmd);
        return;
    }

    cmd->header.header.status = 0;
    if(cmd->header.header.requester == st.st_uid) {
        if(!(st.st_mode & S_IXUSR)) cmd->header.header.status = -EPERM;
    } else {
        if(!(st.st_mode & S_IXOTH)) cmd->header.header.status = -EPERM;
    }

    if(cmd->header.header.status) {
        close(fd);
        luxSendLumen(cmd);
        return;
    }

    // now read the file into memory
    // allocate a new buffer for this
    ExecCommand *res = calloc(1, sizeof(ExecCommand) + st.st_size);
    if(!res) {
        close(fd);
        cmd->header.header.status = -ENOMEM;
        luxSendLumen(cmd);
        return;
    }

    memcpy(res, cmd, sizeof(ExecCommand));

    if(read(fd, res->elf, st.st_size) != st.st_size) {
        close(fd);
        free(res);
        cmd->header.header.status = -1*errno;
        luxSendLumen(cmd);
        return;
    }

    // and relay the response
    res->header.header.length += st.st_size;
    res->header.header.status = 0;
    luxSendLumen(res);
    free(res);
}