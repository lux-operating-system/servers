/*
 * luxOS - a unix-like operating system
 * Omar Elghoul, 2024
 * 
 * pci: Driver and enumerator for PCI (Express)
 */

#include <pci/pci.h>
#include <liblux/liblux.h>

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

        char *classString = NULL;

        switch(class) {
        case 0x01: if(subclass < msdSize) classString = msd[subclass]; break;
        case 0x02: if(subclass < networkSize) classString = network[subclass]; break;
        case 0x03: if(subclass < displaySize) classString = display[subclass]; break;
        case 0x06: if(subclass < bridgeSize) classString = bridge[subclass]; break;
        case 0x0C: if(subclass == 3 && (progif>>4) < usbSize) classString = usb[progif >> 4]; break;
        }

        if(classString)
            luxLogf(KPRINT_LEVEL_DEBUG, "%02x:%02x:%02x - %04X:%04X - class %d:%d (%s)\n", bus, slot, function, vendor, device, class, subclass, classString);
        else
            luxLogf(KPRINT_LEVEL_DEBUG, "%02x:%02x:%02x - %04X:%04X - class %d:%d (unsupported device)\n", bus, slot, function, vendor, device, class, subclass, classString);

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