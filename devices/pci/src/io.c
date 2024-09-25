/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * pci: Driver and enumerator for PCI (Express)
 */

#include <pci/pci.h>
#include <liblux/liblux.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

/* File name format: /pci/bb.ss.ff/[register]
 *
 * Registers with file system representation:
 *  vendor      4
 *  device      4
 *  class       6
 *  classname   variable-length
 *  hdrtype     2
 *  command     4
 *  barX        16
 *  barXsize    16
 *  barXraw     16
 * 
 * All registers are expressed in their ASCII representation
 */

/* parseHex(): parses a hex string
 * params: str - source string
 * returns: numerical value
 */

uint64_t parseHex(const char *str) {
    uint64_t num = 0;
    size_t s = 0;

    for(int i = 0; i < strlen(str); i++) {
        if((str[i] >= '0' && str[i] <= '9') ||
            (str[i] >= 'a' && str[i] <= 'f') ||
            (str[i] >= 'A' && str[i] <= 'F')) s++;
        else
            break;
    }

    if(!s) return 0;

    uint8_t digit;

    for(int i = s - 1; i >= 0; i--) {
        if(str[i] >= '0' && str[i] <= '9') digit = str[i] - '0';
        else if(str[i] >= 'a' && str[i] <= 'f') digit = str[i] - 'a' + 10;
        else if(str[i] >= 'A' && str[i] <= 'F') digit = str[i] - 'A' + 10;
        else digit = 0;

        uint8_t shift = (s - i - 1) << 4;
        num |= (digit << shift);
    }

    return num;
}

/* pciParseAddress(): parses a string in the format of bb.ss.ff
 * params: str - source string
 * params: bus - destination buffer for the bus
 * params: slot - destination buffer for the slot
 * params: function - destination buffer for the function
 * returns: nothing
 */

static void pciParseAddress(const char *str, uint8_t *bus, uint8_t *slot, uint8_t *function) {
    *bus = parseHex(str);
    *slot = parseHex(str+3);
    *function = parseHex(str+6);
}

/* pciReadFile(): reads from a PCI configuration space file under /dev/pci/
 * params: rcmd - read command message
 * returns: nothing, response relayed to devfs
 */

void pciReadFile(RWCommand *rcmd) {
    rcmd->header.header.response = 1;
    rcmd->header.header.length = sizeof(RWCommand);

    PCIFile *file = pciFindFile(rcmd->path);
    if(!file) {
        rcmd->header.header.status = -ENOENT;
        rcmd->length = 0;
        luxSendDependency(rcmd);
        return;
    }

    if(rcmd->position >= file->size) {
        rcmd->header.header.status = -EIO;
        rcmd->length = 0;
        luxSendDependency(rcmd);
        return;
    }

    size_t truelen;
    if((rcmd->position+rcmd->length) > file->size) truelen = file->size - rcmd->position;
    else truelen = rcmd->length;

    memcpy(rcmd->data, file->data, truelen);
    rcmd->length = truelen;
    rcmd->header.header.status = truelen;
    rcmd->header.header.length += truelen;
    luxSendDependency(rcmd);
}