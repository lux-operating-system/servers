/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * pci: Driver and enumerator for PCI (Express)
 */

#include <pci/pci.h>
#include <liblux/liblux.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

static char *msd[] = {
    "SCSI bus controller",
    "IDE controller",
    "floppy controller",   // seriously? PCI floppies?
    "IPI bus controller",
    "RAID controller",
    "ATA controller",
    "SATA controller",
    "serial SCSI controller",
    "NVM controller"
};

static int msdSize = 9;

static char *network[] = {
    "ethernet controller",
};

static int networkSize = 1;

static char *display[] = {
    "VGA-compatible controller",
    "XGA controller",
    "non-VGA 3D controller"
};

static int displaySize = 3;

static char *bridge[] = {
    "host bridge",
    "ISA bridge",
    "EISA bridge",
    "MCA bridge",
    "PCI-to-PCI bridge",
    "PCMCIA bridge",
    "NuBus bridge",
    "CardBus bridge",
    "Raceway bridge",
    "PCI-to-PCI bridge",
};

static int bridgeSize = 10;

static char *usb[] = {
    "USB 1.1 UHCI controller",
    "USB 1.1 OHCI controller",
    "USB 2.0 EHCI controller",
    "USB 3.x xHCI controller",
};

static int usbSize = 4;

uint64_t pciGetBarSize(uint8_t bus, uint8_t slot, uint8_t function, int bar, bool second) {
    uint8_t reg = PCI_BAR0 + (bar << 2);

    uint32_t og = pciReadDword(bus, slot, function, reg);
    uint64_t size = 0;
    if(og & 1) {
        // I/O bus
        pciWriteDword(bus, slot, function, reg, 0xFFFFFFFF);

        uint32_t temp = ~(pciReadDword(bus, slot, function, reg) & 0xFFFFFFFC);
        temp++;
        pciWriteDword(bus, slot, function, reg, og);

        size = temp;
    } else {
        // memory-mapped I/O
        int type = (og >> 1) & 3;
        if(type == 2) {
            // 64-bit mmio
            if(!second) {
                size = pciGetBarSize(bus, slot, function, bar+1, true);
                size <<= 32;
            }

            pciWriteDword(bus, slot, function, reg, 0xFFFFFFFF);
            uint32_t temp = ~(pciReadDword(bus, slot, function, reg) & 0xFFFFFFF0);

            temp &= 0xFFFFFFF0;
            temp++;
            size |= temp;

            pciWriteDword(bus, slot, function, reg, og);
        } else {
            // 32-bit or 16-bit mmio
            pciWriteDword(bus, slot, function, reg, 0xFFFFFFFF);
            uint32_t temp = ~(pciReadDword(bus, slot, function, reg) & 0xFFFFFFF0);
            temp++;
            size = temp;

            pciWriteDword(bus, slot, function, reg, og);
        }
    }

    return size;
}

void pciDumpGeneral(uint8_t bus, uint8_t slot, uint8_t function) {
    uint16_t subvendor = pciReadWord(bus, slot, function, PCI_SUBSYSTEM_VENDOR);
    uint16_t subdevice = pciReadWord(bus, slot, function, PCI_SUBSYSTEM_DEVICE);
    uint8_t interrupt = pciReadByte(bus, slot, function, PCI_INT_LINE);
    uint8_t pin = pciReadByte(bus, slot, function, PCI_INT_PIN);

    pciCreateFile(bus, slot, function, "subvendor", 4);
    pciCreateFile(bus, slot, function, "subdevice", 4);
    pciCreateFile(bus, slot, function, "intline", 2);
    pciCreateFile(bus, slot, function, "intpin", 2);

    luxLogf(KPRINT_LEVEL_DEBUG, "%02x.%02x.%02x:  subsystem %04X:%04X: irq line %d pin %c%c\n",
        bus, slot, function,
        subvendor, subdevice, interrupt,
        pin >= 1 && pin <= 4 ? '#' : '-',
        pin >= 1 && pin <= 4 ? pin+'A'-1 : '-');

    uint64_t bars[5];
    uint64_t barSizes[5];
    uint64_t base[5];

    char buffer[16];

    for(int i = 0; i < 5; i++) {
        bars[i] = pciReadDword(bus, slot, function, PCI_BAR0 + (i << 2));
        barSizes[i] = pciGetBarSize(bus, slot, function, i, false);

        if(barSizes[i] && bars[i] & 1) {
            // I/O
            base[i] = bars[i] & 0xFFFFFFFC;
            luxLogf(KPRINT_LEVEL_DEBUG, "%02x.%02x.%02x:  bar%d: %s at [0x%04X - 0x%04X]\n", bus, slot, function, i,
                bars[i] & 1 ? "i/o ports" : "memory", base[i], base[i]+barSizes[i]-1);
        } else if(barSizes[i]) {
            // mmio
            base[i] = bars[i] & 0xFFFFFFFFFFFFFFFC;
            luxLogf(KPRINT_LEVEL_DEBUG, "%02x.%02x.%02x:  bar%d: %s at [0x%08X - 0x%08X] %s %s\n",
                bus, slot, function, i,
                bars[i] & 1 ? "i/o ports" : "memory", base[i], base[i]+barSizes[i]-1,
                bars[i] & 2 ? "64-bit" : "32-bit", bars[i] & 8 ? "prefetchable" : "");
        }

        if(barSizes[i]) {
            sprintf(buffer, "bar%draw", i);
            pciCreateFile(bus, slot, function, buffer, 16);  // 16 hex digits, raw value
            sprintf(buffer, "bar%d", i);
            pciCreateFile(bus, slot, function, buffer, 16);  // 16 hex digits, base address
            sprintf(buffer, "bar%dsize", i);
            pciCreateFile(bus, slot, function, buffer, 16);  // 16 hex digits, length
        }
    }
}

void pciEnumerate() {
    uint8_t bus = 0, slot = 0, function = 0;

    do {
        uint16_t vendor = pciReadWord(bus, slot, function, PCI_VENDOR);
        uint16_t device = pciReadWord(bus, slot, function, PCI_DEVICE);

        if(!vendor || vendor == 0xFFFF) {
            if(function) {
                function++;
                if(function > 7) {
                    function = 0;
                    slot++;
                    if(slot > 31) {
                        slot = 0;
                        bus++;
                    }
                }

                continue;
            } else {
                break;
            }
        }

        uint8_t header = pciReadByte(bus, slot, function, PCI_HEADER_TYPE);
        uint8_t class = pciReadByte(bus, slot, function, PCI_CLASS);
        uint8_t subclass = pciReadByte(bus, slot, function, PCI_SUBCLASS);
        uint8_t progif = pciReadByte(bus, slot, function, PCI_PROG_IF);

        pciCreateFile(bus, slot, function, "class", 6);     // 6 hex digits
        pciCreateFile(bus, slot, function, "progif", 2);    // 2 hex digits
        pciCreateFile(bus, slot, function, "vendor", 4);
        pciCreateFile(bus, slot, function, "device", 4);
        pciCreateFile(bus, slot, function, "hdrtype", 2);   // 2 hex digits

        char *classString = NULL;

        switch(class) {
        case 0x01:
            if(subclass < msdSize) classString = msd[subclass];
            else classString = "unimplemented storage controller";
            break;
        case 0x02:
            if(subclass < networkSize) classString = network[subclass];
            else classString = "unimplemented network controller";
            break;
        case 0x03:
            if(subclass < displaySize) classString = display[subclass];
            else classString = "unimplemented display controller";
            break;
        case 0x06:
            if(subclass < bridgeSize) classString = bridge[subclass];
            else classString = "unimplemented bridge";
            break;
        case 0x0C:
            if(subclass == 3 && (progif>>4) < usbSize) classString = usb[progif >> 4];
            else classString = "unimplemented USB host controller";
            break;
        }

        if(classString) {
            luxLogf(KPRINT_LEVEL_DEBUG, "%02x.%02x.%02x: %s: %02x%02x%02x (%04X:%04X):\n", bus, slot, function, classString, class, subclass, progif, vendor, device);
            pciCreateFile(bus, slot, function, "classname", strlen(classString));
        } else {
            luxLogf(KPRINT_LEVEL_DEBUG, "%02x.%02x.%02x: unimplemented device: %02x%02x%02x (%04X:%04X):\n", bus, slot, function, class, subclass, progif, vendor, device);
        }

        switch(header & PCI_HAS_FUNCTIONS) {
        case PCI_GENERAL_DEVICE: pciDumpGeneral(bus, slot, function); break;
        }

        function++;
        if(function > 7) {
            function = 0;
            slot++;
            if(slot > 31) {
                slot = 0;
                bus++;
            }
        }
    } while(1);
}