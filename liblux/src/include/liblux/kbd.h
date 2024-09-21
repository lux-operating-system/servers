/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * liblux: Library abstracting kernel-server communication protocols
 */

#pragma once

/* Keyboard Press Macros
 * These will be used to generalize across different types of keyboards, and
 * the driver is responsible for translating hardware scancodes into these
 */

#define KBD_KEY_RELEASE         0x8000  /* this bit will be set on keyup */

/* Base keys, taken from PS/2 scan code set 1 */
#define KBD_KEY_ESC             0x0001
#define KBD_KEY_1               0x0002
#define KBD_KEY_2               0x0003
#define KBD_KEY_3               0x0004
#define KBD_KEY_4               0x0005
#define KBD_KEY_5               0x0006
#define KBD_KEY_6               0x0007
#define KBD_KEY_7               0x0008
#define KBD_KEY_8               0x0009
#define KBD_KEY_9               0x000A
#define KBD_KEY_0               0x000B
#define KBD_KEY_HYPHEN          0x000C
#define KBD_KEY_EQUAL           0x000D
#define KBD_KEY_BACKSPACE       0x000E
#define KBD_KEY_TAB             0x000F
#define KBD_KEY_Q               0x0010
#define KBD_KEY_W               0x0011
#define KBD_KEY_E               0x0012
#define KBD_KEY_R               0x0013
#define KBD_KEY_T               0x0014
#define KBD_KEY_Y               0x0015
#define KBD_KEY_U               0x0016
#define KBD_KEY_I               0x0017
#define KBD_KEY_O               0x0018
#define KBD_KEY_P               0x0019
#define KBD_KEY_SQBRK_OPEN      0x001A
#define KBD_KEY_SQBRK_CLOSE     0x001B
#define KBD_KEY_ENTER           0x001C
#define KBD_KEY_LEFT_CTRL       0x001D
#define KBD_KEY_A               0x001E
#define KBD_KEY_S               0x001F
#define KBD_KEY_D               0x0020
#define KBD_KEY_F               0x0021
#define KBD_KEY_G               0x0022
#define KBD_KEY_H               0x0023
#define KBD_KEY_J               0x0024
#define KBD_KEY_K               0x0025
#define KBD_KEY_L               0x0026
#define KBD_KEY_SEMICOLON       0x0027
#define KBD_KEY_QUOTE           0x0028
#define KBD_KEY_BACKTICK        0x0029
#define KBD_KEY_LEFT_SHIFT      0x002A
#define KBD_KEY_BACKSLASH       0x002B
#define KBD_KEY_Z               0x002C
#define KBD_KEY_X               0x002D
#define KBD_KEY_C               0x002E
#define KBD_KEY_V               0x002F
#define KBD_KEY_B               0x0030
#define KBD_KEY_N               0x0031
#define KBD_KEY_M               0x0032
#define KBD_KEY_COMMA           0x0033
#define KBD_KEY_PERIOD          0x0034
#define KBD_KEY_FORWARD_SLASH   0x0035
#define KBD_KEY_RIGHT_SHIFT     0x0036
#define KBD_KEY_ASTERISK        0x0037
#define KBD_KEY_LEFT_ALT        0x0038
#define KBD_KEY_SPACE           0x0039
#define KBD_KEY_CAPS_LOCK       0x003A
#define KBD_KEY_F1              0x003B
#define KBD_KEY_F2              0x003C
#define KBD_KEY_F3              0x003D
#define KBD_KEY_F4              0x003E
#define KBD_KEY_F5              0x003F
#define KBD_KEY_F6              0x0040
#define KBD_KEY_F7              0x0041
#define KBD_KEY_F8              0x0042
#define KBD_KEY_F9              0x0043
#define KBD_KEY_F10             0x0044
#define KBD_KEY_F11             0x0057
#define KBD_KEY_F12             0x0058
#define KBD_KEY_NUM_LOCK        0x0045
#define KBD_KEY_SCROLL_LOCK     0x0046
#define KBD_KEYPAD_7            0x0047
#define KBD_KEYPAD_8            0x0048
#define KBD_KEYPAD_9            0x0049
#define KBD_KEYPAD_MINUS        0x004A
#define KBD_KEYPAD_4            0x004B
#define KBD_KEYPAD_5            0x004C
#define KBD_KEYPAD_6            0x004D
#define KBD_KEYPAD_PLUS         0x004E
#define KBD_KEYPAD_1            0x004F
#define KBD_KEYPAD_2            0x0050
#define KBD_KEYPAD_3            0x0051
#define KBD_KEYPAD_0            0x0052
#define KBD_KEYPAD_POINT        0x0053

/* Extended keys, internal representation independent of hardware */
#define KBD_PREVIOUS_TRACK      0x0100
#define KBD_NEXT_TRACK          0x0101
#define KBD_RIGHT_CTRL          0x0102
#define KBD_KEYPAD_ENTER        0x0103
#define KBD_MUTE                0x0104
#define KBD_CALCULATOR          0x0105
#define KBD_PLAY                0x0106
#define KBD_STOP                0x0107
#define KBD_VOLUME_DOWN         0x0108
#define KBD_VOLUME_UP           0x0109
#define KBD_WWW                 0x010A
#define KBD_RIGHT_ALT           0x010B
#define KBD_KEY_HOME            0x010C
#define KBD_KEY_UP              0x010D
#define KBD_KEY_PAGE_UP         0x010E
#define KBD_KEY_LEFT            0x010F
#define KBD_KEY_DOWN            0x0110
#define KBD_KEY_RIGHT           0x0111
#define KBD_KEY_PAGE_DOWN       0x0112
#define KBD_KEY_INSERT          0x0113
#define KBD_KEY_DELETE          0x0114
#define KBD_KEY_APPS            0x0115
#define KBD_ACPI_POWER          0x0116
#define KBD_ACPI_SLEEP          0x0117
#define KBD_ACPI_WAKE           0x0118
#define KBD_SCREENSHOT          0x0119
#define KBD_KEY_END             0x011A
#define KBD_KEY_LEFT_GUI        0x011B
#define KBD_KEY_RIGHT_GUI       0x011C
