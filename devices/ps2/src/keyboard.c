/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * ps2: Driver for PS/2 (and USB emulated) keyboards and mice
 */

#include <ps2/ps2.h>
#include <liblux/liblux.h>
#include <sys/io.h>

/* keyboardInit(): initializes a PS/2 keyboard
 * params: none
 * returns: nothing
 */

void keyboardInit() {
    // check if there is a keyboard connected
    if(ps2send(PS2_KEYBOARD, PS2_KEYBOARD_ECHO) != PS2_KEYBOARD_ECHO) return;

    // reset the keyboard
    while(ps2send(PS2_KEYBOARD, PS2_KEYBOARD_RESET) != PS2_DEVICE_ACK);
    while(!readReady());
    uint8_t status = inb(0x60);
    if(status != PS2_DEVICE_PASS) {
        luxLogf(KPRINT_LEVEL_ERROR, "failed to reset PS/2 keyboard, response byte 0x%02X\n", status);
        return;
    }
}