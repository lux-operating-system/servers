/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * vfs: Microkernel server implementing a virtual file system
 */

#include <liblux/liblux.h>

int main(int argc, char **argv) {
    luxInit("vfs");     // this will connect to lux and lumen

    // show signs of life
    luxLog(KPRINT_LEVEL_DEBUG, "virtual file system server started\n");

    while(1);   // todo
}