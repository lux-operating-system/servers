/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * ide: Device driver for IDE (ATA HDDs)
 */

#include <liblux/liblux.h>
#include <liblux/sdev.h>
#include <ide/ide.h>
#include <sys/io.h>

/* ataDelay(): delays the I/O bus by reading from the status port
 * params: base - base I/O port of an IDE controller
 * returns: nothing
 */

void ataDelay(uint16_t base) {
    for(int i = 0; i < 4; i++)
        inb(base + ATA_COMMAND_STATUS);
}
