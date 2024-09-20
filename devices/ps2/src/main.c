/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * ps2: Driver for PS/2 (and USB emulated) keyboards and mice
 */

#include <liblux/liblux.h>
#include <sys/io.h>
#include <ps2/ps2.h>

int main() {
    luxInit("ps2");

    // request access to I/O ports 0x60-0x64
    if(ioperm(0x60, 5, 1)) {
        luxLogf(KPRINT_LEVEL_ERROR, "PS/2 driver failed to acquire I/O ports\n");
        return -1;
    }

    keyboardInit();

    // notify lumen that startup is complete
    luxReady();

    for(;;) {
        IRQCommand irqcmd;
        if(luxRecvKernel(&irqcmd, sizeof(IRQCommand), true) == sizeof(IRQCommand)) {
            if(irqcmd.pin == 1) {
                // keyboard interrupt
                luxLogf(KPRINT_LEVEL_DEBUG, "keyboard IRQ: read scan code 0x%02X\n", inb(0x60));
            }
        }
    }
}