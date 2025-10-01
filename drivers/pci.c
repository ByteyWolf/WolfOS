#include <stdint.h>
#include "pci.h"
#include "pci_names.h"
#include "../util/earlyutil.h"
#include "../console/console.h"

/* helpers (expect outl/inl to be available) */
static inline uint32_t pci_cfg_addr(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    return (1u << 31)                    // enable bit
         | ((uint32_t)bus  << 16)
         | ((uint32_t)slot << 11)
         | ((uint32_t)func << 8)
         | ((uint32_t)offset & 0xFCu);   // align to dword
}

/* dword read */
uint32_t pci_read_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = pci_cfg_addr(bus, slot, func, offset);
    outl(0xCF8, address);
    return inl(0xCFC);
}

/* word read (returns lower 16 bits of chosen half-word) */
uint16_t pci_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = pci_cfg_addr(bus, slot, func, offset);
    outl(0xCF8, address);
    uint32_t val = inl(0xCFC);
    /* offset & 2 selects lower (0) or upper (2) half of the dword -> shift by 0 or 16 */
    uint32_t shift = ((offset & 2u) ? 16u : 0u);
    return (uint16_t)((val >> shift) & 0xFFFFu);
}

/* byte read */
uint8_t pci_read_byte(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = pci_cfg_addr(bus, slot, func, offset);
    outl(0xCF8, address);
    uint32_t val = inl(0xCFC);
    uint32_t shift = (offset & 3u) * 8u; /* 0,8,16,24 */
    return (uint8_t)((val >> shift) & 0xFFu);
}

/* dword write */
void pci_write_dword(uint8_t bus, uint8_t slot, uint8_t func,
                     uint8_t offset, uint32_t value) {
    uint32_t address = pci_cfg_addr(bus, slot, func, offset);
    outl(0xCF8, address);
    outl(0xCFC, value);
}

/* word write (read-modify-write on the underlying dword using inl/outl) */
void pci_write_word(uint8_t bus, uint8_t slot, uint8_t func,
                    uint8_t offset, uint16_t value) {
    uint32_t address = pci_cfg_addr(bus, slot, func, offset);
    outl(0xCF8, address);
    uint32_t current = inl(0xCFC);

    if (offset & 2) {
        /* upper 16 bits */
        current = (current & 0x0000FFFFu) | ((uint32_t)value << 16);
    } else {
        /* lower 16 bits */
        current = (current & 0xFFFF0000u) | (uint32_t)value;
    }

    outl(0xCFC, current);
}
