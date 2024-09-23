/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * pci: Driver and enumerator for PCI (Express)
 */

#include <stdint.h>

#pragma once

#define PCI_CONFIG_ADDRESS          0xCF8
#define PCI_CONFIG_DATA             0xCFC

#define ADDRESS_ENABLE              0x80000000

uint32_t pciReadDword(uint8_t, uint8_t, uint8_t, uint16_t);
uint16_t pciReadWord(uint8_t, uint8_t, uint8_t, uint16_t);
uint8_t pciReadByte(uint8_t, uint8_t, uint8_t, uint16_t);
