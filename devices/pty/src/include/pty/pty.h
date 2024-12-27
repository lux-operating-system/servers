/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * pty: Microkernel server implementing Unix 98-style pseudo-terminal devices
 */

#pragma once

#include <termios.h>
#include <liblux/liblux.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#define MAX_PTYS                4096
#define PTY_BUFFER_INCREMENTS   4096

#define DEFAULT_IFLAG           (ICRNL | IGNCR | IGNPAR)
#define DEFAULT_OFLAG           (ONLRET)
#define DEFAULT_CFLAG           (CS8 | HUPCL)
#define DEFAULT_LFLAG           (ECHO | ECHOE | ECHOK | ECHONL | ICANON)

/* ioctl commands, more to come for controlling terminal size/scroll/etc */
#define PTY_GET_SLAVE           (0x10 | IOCTL_OUT_PARAM)
#define PTY_GRANT_PT            0x20
#define PTY_UNLOCK_PT           0x30
#define PTY_TTY_NAME            (0x40 | IOCTL_OUT_PARAM)

typedef struct {
    // master read() will read from slave, write() will write to master
    // slave read() will read from master, write() will write to slave
    int valid, index, openCount, locked;
    void *master, *slave;
    size_t masterSize, slaveSize;           // buffer size
    size_t masterDataSize, slaveDataSize;   // available data size
    struct termios termios;
} Pty;

extern Pty *ptys;
extern int ptyCount;

void ptyOpen(OpenCommand *);
void ptyOpenMaster(OpenCommand *);
void ptyOpenSlave(OpenCommand *);
void ptyIoctl(IOCTLCommand *);
void ptyIoctlMaster(IOCTLCommand *);
void ptyIoctlSlave(IOCTLCommand *);
void ptyWrite(RWCommand *);
void ptyRead(RWCommand *);
