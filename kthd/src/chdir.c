/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * kthd: Kernel Thread Helper Daemon
 */

#include <liblux/liblux.h>
#include <sys/stat.h>
#include <errno.h>

void kthdChdir(ChdirCommand *cmd) {
    cmd->header.header.response = 1;
    cmd->header.header.length = sizeof(ChdirCommand);

    // simply issue a stat() syscall and ensure the directory is valid
    struct stat st;
    if(stat(cmd->path, &st)) {
        cmd->header.header.status = -1*errno;
        luxSendLumen(cmd);
        return;
    }

    if((st.st_mode & S_IFMT) != S_IFDIR) {
        cmd->header.header.status = -ENOTDIR;
        luxSendLumen(cmd);
        return;
    }

    // ensure the requesting process has execute permissions on the directory
    cmd->header.header.status = 0;
    if(cmd->uid == st.st_uid) {
        if(!(st.st_mode & S_IXUSR)) cmd->header.header.status = -EPERM;
    } else if(cmd->gid == st.st_gid) {
        if(!(st.st_mode & S_IXGRP)) cmd->header.header.status = -EPERM;
    } else {
        if(!(st.st_mode & S_IXOTH)) cmd->header.header.status = -EPERM;
    }

    luxSendLumen(cmd);
}