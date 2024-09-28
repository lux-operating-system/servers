/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * sdev: Abstraction for storage devices under /dev/sdX
 */

#include <liblux/liblux.h>
#include <liblux/sdev.h>
#include <sdev/sdev.h>

int main() {
    luxInit("sdev");
    while(luxConnectDependency("devfs"));   // obviously depend on devfs

    // there really isn't anything to do here until storage device drivers are
    // loaded, so notify lumen that we're ready
    luxReady();

    while(1);
}