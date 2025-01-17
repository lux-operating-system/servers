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
#define DEFAULT_LFLAG           (ECHO | ECHOE | ECHOK | ECHONL | ICANON | ISIG)

/* terminal window sizes should be initialized to an "appropriate" default
 * according to POSIX Issue 8, and so this number makes a little sense
 * https://pubs.opengroup.org/onlinepubs/9799919799/functions/tcgetwinsize.html
 */

#define DEFAULT_WIDTH           80
#define DEFAULT_HEIGHT          25

/* ioctl commands, more to come for controlling terminal size/scroll/etc */
#define PTY_GET_SECONDARY       (0x10 | IOCTL_OUT_PARAM)
#define PTY_GRANT_PT            0x20
#define PTY_UNLOCK_PT           0x30
#define PTY_TTY_NAME            (0x40 | IOCTL_OUT_PARAM)

/* termios commands */
#define PTY_SET_INPUT           (0x50 | IOCTL_IN_PARAM)
#define PTY_SET_OUTPUT          (0x60 | IOCTL_IN_PARAM)
#define PTY_SET_LOCAL           (0x70 | IOCTL_IN_PARAM)
#define PTY_SET_CONTROL         (0x80 | IOCTL_IN_PARAM)

#define PTY_GET_INPUT           (0x90 | IOCTL_OUT_PARAM)
#define PTY_GET_OUTPUT          (0xA0 | IOCTL_OUT_PARAM)
#define PTY_GET_LOCAL           (0xB0 | IOCTL_OUT_PARAM)
#define PTY_GET_CONTROL         (0xC0 | IOCTL_OUT_PARAM)

#define PTY_SET_WINSIZE         (0xD0 | IOCTL_IN_PARAM)
#define PTY_GET_WINSIZE         (0xE0 | IOCTL_OUT_PARAM)

#define PTY_SET_FOREGROUND      (0xF0 | IOCTL_IN_PARAM)
#define PTY_GET_FOREGROUND      (0x100 | IOCTL_OUT_PARAM)

#define PTY_SET_NCSS1           (0x110 | IOCTL_IN_PARAM)
#define PTY_SET_NCSS2           (0x120 | IOCTL_IN_PARAM)
#define PTY_GET_NCSS1           (0x130 | IOCTL_OUT_PARAM)
#define PTY_GET_NCSS2           (0x140 | IOCTL_OUT_PARAM)

/* default control characters */
#define PTY_EOF                 0x04
#define PTY_EOL                 0xFF
#define PTY_ERASE               0x7F
#define PTY_INTR                0x03
#define PTY_KILL                0x15
#define PTY_MIN                 0x01
#define PTY_QUIT                0x1C
#define PTY_START               0x11
#define PTY_STOP                0x13
#define PTY_SUSP                0x1A
#define PTY_TIME                0x00

typedef struct {
    int valid, index, openCount, locked;
    void *primary, *secondary;
    size_t primarySize, secondarySize;           // buffer size
    size_t primaryDataSize, secondaryDataSize;   // available data size
    struct termios termios;
    struct winsize ws;
    pid_t group;    // foreground process group
} Pty;

extern Pty *ptys;
extern int ptyCount;

void ptyOpen(OpenCommand *);
void ptyOpenPrimary(OpenCommand *);
void ptyOpenSecondary(OpenCommand *);
void ptyIoctl(IOCTLCommand *);
void ptyIoctlPrimary(IOCTLCommand *);
void ptyIoctlSecondary(IOCTLCommand *);
void ptyWrite(RWCommand *);
void ptyRead(RWCommand *);
void ptyFsync(FsyncCommand *);