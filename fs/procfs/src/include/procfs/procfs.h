/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * procfs: Microkernel server implementing the /proc file system
 */

#pragma once

#include <liblux/liblux.h>
#include <sys/types.h>

/* for /proc/kernel, /proc/memsize, /proc/memusage, etc */
#define RESOLVE_KERNEL              1
#define RESOLVE_MEMSIZE             2
#define RESOLVE_MEMUSAGE            3
#define RESOLVE_PAGESIZE            4
#define RESOLVE_UPTIME              5
#define RESOLVE_SYS                 6

/* for /proc/pid/X*/
#define RESOLVE_PID                 0x8000
#define RESOLVE_PID_TYPE            (0 | RESOLVE_PID)   /* /proc/pid/type */
#define RESOLVE_PID_CMD             (1 | RESOLVE_PID)   /* /proc/pid/cmd */
#define RESOLVE_PID_CWD             (2 | RESOLVE_PID)   /* /proc/pid/cwd */
#define RESOLVE_PID_MEM             (3 | RESOLVE_PID)   /* /proc/pid/mem */
#define RESOLVE_PID_IOPORTS         (4 | RESOLVE_PID)   /* /proc/pid/ioports */
#define RESOLVE_PID_USER            (5 | RESOLVE_PID)   /* /proc/pid/user */
#define RESOLVE_PID_GROUP           (6 | RESOLVE_PID)   /* /proc/pid/group */
#define RESOLVE_PID_TIDS            (7 | RESOLVE_PID)   /* /proc/pid/tids */
#define RESOLVE_PID_PARENT          (8 | RESOLVE_PID)   /* /proc/pid/parent */
#define RESOLVE_PID_CHILDREN        (9 | RESOLVE_PID)   /* /proc/pid/children */
#define RESOLVE_PID_STAT            (10 | RESOLVE_PID)  /* /proc/pid/stat */

#define RESOLVE_DIRECTORY           0x10000

extern SysInfoResponse *sysinfo;

void procfsMount(MountCommand *);
void procfsStat(StatCommand *);
void procfsOpen(OpenCommand *);
void procfsRead(RWCommand *);
void procfsWrite(RWCommand *);

int resolve(const char *, pid_t *);
