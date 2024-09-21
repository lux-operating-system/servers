/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * ps2: Driver for PS/2 (and USB emulated) keyboards and mice
 */

#pragma once

#include <stdint.h>

#define PS2_CONTROLLER                  0
#define PS2_KEYBOARD                    1
#define PS2_MOUSE                       2

/* PS/2 Controller Commands */
#define PS2_DISABLE_MOUSE               0xA7
#define PS2_ENABLE_MOUSE                0xA8
#define PS2_TEST_MOUSE                  0xA9
#define PS2_TEST_CONTROLLER             0xAA
#define PS2_TEST_KEYBOARD               0xAB
#define PS2_DISABLE_KEYBOARD            0xAD
#define PS2_ENABLE_KEYBOARD             0xAE
#define PS2_MOUSE_COMMAND               0xD4
#define PS2_SYSTEM_RESET                0xFE

/* PS/2 Keyboard Commands */
#define PS2_KEYBOARD_ECHO               0xEE
#define PS2_KEYBOARD_RESET              0xFF
#define PS2_KEYBOARD_SET_AUTOREPEAT     0xF3
#define PS2_KEYBOARD_ENABLE_SCAN        0xF4
#define PS2_KEYBOARD_DISABLE_SCAN       0xF5
#define PS2_KEYBOARD_SET_SCANCODE       0xF0

#define PS2_KEYBOARD_SCANCODE           2       /* use scan code set 2 */

/* PS/2 Device Responses */
#define PS2_DEVICE_ACK                  0xFA
#define PS2_DEVICE_RESEND               0xFE
#define PS2_DEVICE_PASS                 0xAA

int readReady();
int writeReady();

uint8_t ps2send(int, uint8_t);
void ps2sendNoACK(int, uint8_t);

void keyboardInit();
