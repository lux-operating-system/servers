/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * kthd: Kernel Thread Helper Daemon
 */

#include <liblux/liblux.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

/* clean(): helper function to clean up a path
 * params: path - path to be cleaned, the string will be directly modified
 * params: pointer to path
 */

static char *clean(char *path) {
    if(strlen(path) == 1) return path;  // root will never need to be cleaned

    // first try to get rid of excessive slashes
    for(int i = 0; i < strlen(path); i++) {
        if((path[i] == '/') && (i < strlen(path)-1) && (path[i+1] == '/')) {
            memmove(&path[i], &path[i+1], strlen(&path[i])+1);
            continue;
        }
    }

    // if the last character is a slash, remove it except for the root dir
    if(strlen(path) == 1) return path;
    while(path[strlen(path)-1] == '/') path[strlen(path)-1] = 0;

    // parse '../' to reflect the parent directory
    for(int i = 0; i < strlen(path); i++) {
        if((path[i] == '.') && (path[i+1] == '.') && ((path[i+2] == '/') || (!path[i+2]))) {
            // find the parent
            int parent = i-2;
            for(; parent > 0; parent--) {
                if(path[parent] == '/') break;
            }

            parent++;
            memmove(&path[parent], &path[i+3], strlen(&path[i+3])+1);
        }
    }

    // remove './' because it refers to the self directory
    for(int i = 0; i < strlen(path); i++) {
        if((path[i] == '.') && ((path[i+1] == '/') || (!path[i+1]))) {
            memmove(&path[i], &path[i+2], strlen(&path[i+2])+1);
            continue;
        }
    }

    // and finally remove any trailing slashes left while processing the string
    if(strlen(path) == 1) return path;
    while(path[strlen(path)-1] == '/') path[strlen(path)-1] = 0;

    return path;
}

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

    // success, clean up the path
    clean(cmd->path);
    luxSendLumen(cmd);
}