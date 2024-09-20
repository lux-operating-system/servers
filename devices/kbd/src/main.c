/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * kbd: Abstraction for keyboard devices under /dev/kbd
 */

#include <liblux/liblux.h>

int main() {
    luxInit("kbd");
    while(luxConnectDependency("devfs"));

    luxLogf(KPRINT_LEVEL_DEBUG, "sign of life\n");
    while(1);
}