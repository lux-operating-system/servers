/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * liblux: Library abstracting kernel-server communication protocols
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <sys/types.h>

#define SERVER_KERNEL_PATH      "lux:///kernel"     // not a real file, special path
#define SERVER_LUMEN_PATH       "lux:///lumen"      // likewise not a real file

/* these commands are requested by lumen and the servers and fulfilled by the kernel */
#define COMMAND_LOG             0x0000  // output to kernel log
#define COMMAND_SYSINFO         0x0001  // get uptime, mem usage, etc
#define COMMAND_RAND            0x0002  // random number
#define COMMAND_IO              0x0003  // request I/O access
#define COMMAND_PROCESS_IO      0x0004  // get process I/O privileges
#define COMMAND_PROCESS_LIST    0x0005  // get list of processes/threads
#define COMMAND_PROCESS_STATUS  0x0006  // get status of process/thread
#define COMMAND_FRAMEBUFFER     0x0007  // request frame buffer access

#define MAX_GENERAL_COMMAND     0x0007

/* these commands are requested by the kernel for lumen to fulfill syscall requests */
#define COMMAND_STAT            0x8000
#define COMMAND_FLUSH           0x8001
#define COMMAND_MOUNT           0x8002
#define COMMAND_UMOUNT          0x8003

#define MAX_SYSCALL_COMMAND     0x0003

#define KPRINT_LEVEL_DEBUG      0
#define KPRINT_LEVEL_WARNING    1
#define KPRINT_LEVEL_ERROR      2
#define KPRINT_LEVEL_PANIC      3

typedef struct {
    uint16_t command;
    uint16_t length;
    uint8_t response;       // 0 for requests, 1 for response
    uint8_t reserved[3];    // for alignment
    uint64_t latency;       // in ms, for responses
    uint64_t status;        // return value for responses
    pid_t requester;
} MessageHeader;

typedef struct {
    MessageHeader header;
    uint64_t id;            // syscall request ID
} SyscallHeader;

/* sysinfo command */
typedef struct {
    MessageHeader header;
    char kernel[64];        // version string
    uint64_t uptime;
    int maxPid, maxSockets, maxFiles;
    int processes, threads;
    int pageSize;
    int memorySize, memoryUsage;    // in pages
} SysInfoResponse;

/* log command */
typedef struct {
    MessageHeader header;
    int level;
    char server[512];       // null terminated
    char message[];         // variable length, null terminated
} LogCommand;

/* framebuffer access command */
typedef struct {
    MessageHeader header;
    uint64_t buffer;        // pointer
    uint16_t w, h, pitch, bpp;
} FramebufferResponse;

/* wrapper functions */
pid_t luxGetSelf();
const char *luxGetName();
int luxInit(const char *);
int luxInitLumen();
int luxConnectKernel();
int luxConnectLumen();
int luxGetKernelSocket();
ssize_t luxSendKernel(void *);
ssize_t luxRecvKernel(void *, size_t, bool);
void luxLog(int, const char *);
int luxRequestFramebuffer(FramebufferResponse *);
