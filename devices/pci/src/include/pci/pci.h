/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * pci: Driver and enumerator for PCI (Express)
 */

#include <stdint.h>
#include <stddef.h>
#include <liblux/liblux.h>

#pragma once

#define PCI_CONFIG_ADDRESS          0xCF8
#define PCI_CONFIG_DATA             0xCFC

#define ADDRESS_ENABLE              0x80000000

/* PCI Configuration Space Registers */
#define PCI_VENDOR                  0x00    // word
#define PCI_DEVICE                  0x02    // word
#define PCI_COMMAND                 0x04    // word
#define PCI_STATUS                  0x06    // word
#define PCI_REVID                   0x08    // byte
#define PCI_PROG_IF                 0x09    // byte
#define PCI_SUBCLASS                0x0A    // byte
#define PCI_CLASS                   0x0B    // byte
#define PCI_CACHE_LINE              0x0C    // byte
#define PCI_LATENCY                 0x0D    // byte
#define PCI_HEADER_TYPE             0x0E    // byte
#define PCI_BIST                    0x0F    // byte

/* PCI Header Type */
#define PCI_HAS_FUNCTIONS           0x80
#define PCI_GENERAL_DEVICE          0x00
#define PCI_TO_PCI_BRIDGE           0x01
#define PCI_TO_CARDBUS_BRIDGE       0x02

/* PCI Configuration Space Registers for General Devices */
#define PCI_BAR0                    0x10    // dword
#define PCI_BAR1                    0x14    // dword
#define PCI_BAR2                    0x18    // dword
#define PCI_BAR3                    0x1C    // dword
#define PCI_BAR4                    0x20    // dword
#define PCI_BAR5                    0x24    // dword
#define PCI_CARDBUS_POINTER         0x28    // dword
#define PCI_SUBSYSTEM_VENDOR        0x2C    // word
#define PCI_SUBSYSTEM_DEVICE        0x2E    // word
#define PCI_EXPANSION_ROM           0x30    // dword
#define PCI_CAPABILITIES            0x34    // byte
#define PCI_INT_LINE                0x3C    // byte
#define PCI_INT_PIN                 0x3D    // byte
#define PCI_MIN_GRANT               0x3E    // byte
#define PCI_MAX_LATENCY             0x3F    // byte

typedef struct PCIFile {
    char name[32];
    size_t size;
    uint16_t reg;
    uint8_t data[256];
    struct PCIFile *next;
} PCIFile;

uint32_t pciReadDword(uint8_t, uint8_t, uint8_t, uint16_t);
uint16_t pciReadWord(uint8_t, uint8_t, uint8_t, uint16_t);
uint8_t pciReadByte(uint8_t, uint8_t, uint8_t, uint16_t);
void pciWriteDword(uint8_t, uint8_t, uint8_t, uint16_t, uint32_t);
void pciWriteWord(uint8_t, uint8_t, uint8_t, uint16_t, uint16_t);
void pciWriteByte(uint8_t, uint8_t, uint8_t, uint16_t, uint8_t);

void pciEnumerate();
void pciCreateFile(uint8_t, uint8_t, uint8_t, uint16_t, int, const char *, size_t, void *);
void pciReadFile(RWCommand *);
void pciWriteFile(RWCommand *);
