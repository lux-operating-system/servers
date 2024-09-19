/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * ps2: Driver for PS/2 (and USB emulated) keyboards and mice
 */

#pragma once

#include <stdint.h>

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

int readReady();
int writeReady();

uint8_t ps2send(int, uint8_t);
void ps2sendNoACK(int, uint8_t);