/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * ps2: Driver for PS/2 (and USB emulated) keyboards and mice
 */

#include <string.h>
#include <liblux/liblux.h>
#include <liblux/kbd.h>
#include <sys/io.h>
#include <ps2/ps2.h>

int main() {
    luxInit("ps2");
    while(luxConnectDependency("kbd"));     // depend on /dev/kbd

    // request access to I/O ports 0x60-0x64
    if(ioperm(0x60, 5, 1)) {
        luxLogf(KPRINT_LEVEL_ERROR, "PS/2 driver failed to acquire I/O ports\n");
        return -1;
    }

    keyboardInit();

    // notify lumen that startup is complete
    luxReady();

    MessageHeader msg;
    memset(&msg, 0, sizeof(MessageHeader));
    msg.command = 0xFFFF;       // special keyboard driver command
    msg.length = sizeof(MessageHeader);

    int extended = 0;      // 0 for normal keys, 1 for extended keys

    for(;;) {
        IRQCommand irqcmd;
        if(luxRecvKernel(&irqcmd, sizeof(IRQCommand), true, false) == sizeof(IRQCommand)) {
            if(irqcmd.pin == 1) {
                // keyboard interrupt handler
                uint16_t key = 0;
                uint8_t code = inb(0x60);

                // extended keys have a prefix byte
                if(code == 0xE0) {
                    extended = 1;
                    continue;
                }

                // handle extended keys
                if(extended) {
                    switch(code & 0x7F) {
                    case 0x10: key = KBD_PREVIOUS_TRACK; break;
                    case 0x19: key = KBD_NEXT_TRACK; break;
                    case 0x1C: key = KBD_KEYPAD_ENTER; break;
                    case 0x1D: key = KBD_RIGHT_CTRL; break;
                    case 0x20: key = KBD_MUTE; break;
                    case 0x21: key = KBD_CALCULATOR; break;
                    case 0x22: key = KBD_PLAY; break;
                    case 0x24: key = KBD_STOP; break;
                    case 0x2E: key = KBD_VOLUME_DOWN; break;
                    case 0x30: key = KBD_VOLUME_UP; break;
                    case 0x32: key = KBD_WWW; break;
                    case 0x38: key = KBD_RIGHT_ALT; break;
                    case 0x47: key = KBD_KEY_HOME; break;
                    case 0x48: key = KBD_KEY_UP; break;
                    case 0x49: key = KBD_KEY_PAGE_UP; break;
                    case 0x4B: key = KBD_KEY_LEFT; break;
                    case 0x4D: key = KBD_KEY_RIGHT; break;
                    case 0x4F: key = KBD_KEY_END; break;
                    case 0x50: key = KBD_KEY_DOWN; break;
                    case 0x51: key = KBD_KEY_PAGE_DOWN; break;
                    case 0x52: key = KBD_KEY_INSERT; break;
                    case 0x53: key = KBD_KEY_DELETE; break;
                    case 0x5B: key = KBD_KEY_LEFT_GUI; break;
                    case 0x5C: key = KBD_KEY_RIGHT_GUI; break;
                    case 0x5D: key = KBD_KEY_APPS; break;
                    case 0x5E: key = KBD_ACPI_POWER; break;
                    case 0x5F: key = KBD_ACPI_SLEEP; break;
                    case 0x63: key = KBD_ACPI_WAKE; break;
                    case 0x2A:
                    case 0x37: key = KBD_SCREENSHOT;
                    }
                } else {
                    // normal keys
                    key = code & 0x7F;
                }
                
                if(key) {
                    if(code & 0x80) key |= KBD_KEY_RELEASE;
                    msg.status = key;
                    luxSendDependency(&msg);
                }
            }
        }
    }
}