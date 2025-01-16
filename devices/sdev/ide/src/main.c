/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * ide: Device driver for IDE (ATA HDDs)
 */

#include <liblux/liblux.h>
#include <liblux/sdev.h>
#include <ide/ide.h>
#include <unistd.h>

int main(void) {
    luxInit("ide");
    while(luxConnectDependency("sdev"));

    // TODO
    luxReady();

    for(;;) {
        sched_yield();
    }
}