/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * procfs: Microkernel server implementing the /proc file system
 */

#include <procfs/procfs.h>
#include <string.h>
#include <stdlib.h>

/* resolve(): resolves a path on the /proc file system
 * params: path - file name
 * params: pid - destination to store pid buffer for /proc/pid/X
 * returns: type of resolved path, -1 if non-existent
 */

int resolve(const char *path, pid_t *pid) {
    if(!path) return -1;
    if(!strcmp(path, "/sys")) return RESOLVE_SYS;
    if(!strcmp(path, "/kernel")) return RESOLVE_KERNEL;
    if(!strcmp(path, "/memsize")) return RESOLVE_MEMSIZE;
    if(!strcmp(path, "/memusage")) return RESOLVE_MEMUSAGE;
    if(!strcmp(path, "/pagesize")) return RESOLVE_PAGESIZE;
    if(!strcmp(path, "/uptime")) return RESOLVE_UPTIME;

    // TODO
    return -1;
}