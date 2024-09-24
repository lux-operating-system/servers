/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * pci: Driver and enumerator for PCI (Express)
 */

#include <pci/pci.h>
#include <sys/io.h>
#include <liblux/liblux.h>

uint32_t pciReadDword(uint8_t bus, uint8_t slot, uint8_t function, uint16_t offset) {
    uint32_t address = (bus << 16) | ((slot & 0x1F) << 11) | ((function & 7) << 8) | (offset & 0xFC);
    outd(PCI_CONFIG_ADDRESS, address | ADDRESS_ENABLE);
    return ind(PCI_CONFIG_DATA);
}

uint16_t pciReadWord(uint8_t bus, uint8_t slot, uint8_t function, uint16_t offset) {
    uint32_t dword = pciReadDword(bus, slot, function, offset);
    offset &= 3;
    return (uint32_t) dword >> (offset << 3);
}

uint8_t pciReadByte(uint8_t bus, uint8_t slot, uint8_t function, uint16_t offset) {
    uint32_t dword = pciReadDword(bus, slot, function, offset);
    offset &= 7;
    return (uint32_t) dword >> (offset << 3);
}

void pciWriteDword(uint8_t bus, uint8_t slot, uint8_t function, uint16_t offset, uint32_t data) {
    uint32_t address = (bus << 16) | ((slot & 0x1F) << 11) | ((function & 7) << 8) | (offset & 0xFC);
    outd(PCI_CONFIG_ADDRESS, address | ADDRESS_ENABLE);
    outd(PCI_CONFIG_DATA, data);
}

void pciWriteWord(uint8_t bus, uint8_t slot, uint8_t function, uint16_t offset, uint16_t data) {
    uint32_t dword = pciReadDword(bus, slot, function, offset);
    uint16_t offsetShift = offset & 3;
    uint32_t mask = 0xFFFF << (offsetShift << 3);
    mask = ~mask;

    dword &= ~mask;
    dword |= (data << (offsetShift << 8));
    pciWriteDword(bus, slot, function, offset, data);
}
