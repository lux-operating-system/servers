/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * ps2: Driver for PS/2 (and USB emulated) keyboards and mice
 */

#include <ps2/ps2.h>
#include <sys/io.h>

/* readReady(): checks if the PS/2 controller is ready to read data
 * params: none
 * returns: 0 if not ready, 1 if ready
 */

int readReady() {
    return inb(0x64) & 1;
}

/* writeReady(): checks if the PS/2 controller is ready to write data
 * params: none
 * returns: 0 if not ready, 1 if ready
 */

int writeReady() {
    return (inb(0x64) >> 1) & 1;
}

/* ps2send(): sends a command to the PS/2 controller
 * params: dev - 0 for controller, 1 for keyboard, 2 for mouse
 * params: cmd - command byte
 * returns: acknowledgement byte
 */

uint8_t ps2send(int dev, uint8_t cmd) {
    ps2sendNoACK(dev, cmd);

    while(!readReady());
    return inb(0x60);
}

/* ps2sendNoACK(): sends a command to the PS/2 controller without acknowledgement
 * params: dev - 0 for controller, 1 for keyboard, 2 for mouse
 * params: cmd - command byte
 * returns: nothing
 */

void ps2sendNoACK(int dev, uint8_t cmd) {
    while(!writeReady());
    if(!dev)
        outb(0x64, cmd);    // controller
    else if(dev == 1)
        outb(0x60, cmd);    // keyboard
    else {
        // mouse
        outb(0x64, PS2_MOUSE_COMMAND);
        while(!writeReady());
        outb(0x60, cmd);
    }
}
