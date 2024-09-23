/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * liblux: Library abstracting kernel-server communication protocols
 */

#pragma once

#include <sys/ioctl.h>

#define LFB_GET_WIDTH           (0x10 | IOCTL_OUT_PARAM)
#define LFB_GET_HEIGHT          (0x20 | IOCTL_OUT_PARAM)
