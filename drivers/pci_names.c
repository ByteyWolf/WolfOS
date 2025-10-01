#include "pci_names.h"

/* Base class names (0x00..0xFF, sparse). */
static const char *class_names[256] = {
    [0x00] = "Unclassified",
    [0x01] = "Mass Storage Controller",
    [0x02] = "Network Controller",
    [0x03] = "Display Controller",
    [0x04] = "Multimedia Controller",
    [0x05] = "Memory Controller",
    [0x06] = "Bridge Device",
    [0x07] = "Simple Communications Controller",
    [0x08] = "Base System Peripheral",
    [0x09] = "Input Device",
    [0x0A] = "Docking Station",
    [0x0B] = "Processor",
    [0x0C] = "Serial Bus Controller",
    [0x0D] = "Wireless Controller",
    [0x0E] = "Intelligent I/O Controller",
    [0x0F] = "Satellite Communications Controller",
    [0x10] = "Encryption/Decryption Controller",
    [0x11] = "Signal Processing Controller",
    [0x12] = "Processing Accelerator",
    [0x13] = "Non-Essential Instrumentation",
    [0xFF] = "Unassigned/Unknown"
};

/* Subclass names indexed by class, then subclass */
static const char *subclass_names[256][256] = {
    [0x01] = { /* Mass Storage Controller */
        [0x00] = "SCSI Bus Controller",
        [0x01] = "IDE Controller",
        [0x02] = "Floppy Disk Controller",
        [0x03] = "IPI Bus Controller",
        [0x04] = "RAID Controller",
        [0x05] = "ATA Controller (Single DMA)",
        [0x06] = "Serial ATA (SATA) Controller",
        [0x07] = "Serial Attached SCSI (SAS) Controller",
        [0x08] = "Non-Volatile Memory Controller (NVMe)",
        [0x80] = "Other Mass Storage Controller"
    },
    [0x02] = { /* Network Controller */
        [0x00] = "Ethernet Controller",
        [0x01] = "Token Ring Controller",
        [0x02] = "FDDI Controller",
        [0x03] = "ATM Controller",
        [0x04] = "ISDN Controller",
        [0x80] = "Other Network Controller"
    },
    [0x03] = { /* Display Controller */
        [0x00] = "VGA Compatible Controller",
        [0x01] = "XGA Controller",
        [0x02] = "3D Controller (Not VGA-Compatible)",
        [0x80] = "Other Display Controller"
    },
    [0x06] = { /* Bridge Device */
        [0x00] = "Host Bridge",
        [0x01] = "ISA Bridge",
        [0x02] = "EISA Bridge",
        [0x03] = "MCA Bridge",
        [0x04] = "PCI-to-PCI Bridge",
        [0x05] = "PCMCIA Bridge",
        [0x06] = "NuBus Bridge",
        [0x07] = "CardBus Bridge",
        [0x08] = "RACEway Bridge",
        [0x09] = "PCI-to-PCI Bridge (Semi-Transparent)",
        [0x0A] = "InfiniBand-to-PCI Host Bridge",
        [0x80] = "Other Bridge Device"
    },
    [0x0C] = { /* Serial Bus Controller */
        [0x00] = "FireWire (IEEE 1394) Controller",
        [0x01] = "ACCESS Bus Controller",
        [0x02] = "SSA Controller",
        [0x03] = "USB Controller",
        [0x04] = "Fibre Channel Controller",
        [0x05] = "SMBus Controller",
        [0x06] = "InfiniBand Controller",
        [0x07] = "IPMI Interface",
        [0x08] = "SERCOS Interface",
        [0x09] = "CANbus Controller",
        [0x80] = "Other Serial Bus Controller"
    }
    /* ... expand more if needed ... */
};

const char *pci_class_name(uint8_t class_code) {
    const char *s = class_names[class_code];
    return s ? s : "Reserved/Unknown";
}

const char *pci_subclass_name(uint8_t class_code, uint8_t subclass_code) {
    const char *s = subclass_names[class_code][subclass_code];
    return s ? s : "Reserved/Unknown";
}
