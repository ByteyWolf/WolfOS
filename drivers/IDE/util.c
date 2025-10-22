#include <stdint.h>
#include "definitions.h"

// thanks osdev.org

void ide_write(struct ide_channel* channel, uint8_t reg, uint8_t data) {
    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, ATA_REG_CONTROL, 0x80 | channel->nIEN);
    if (reg < 0x08)
        outb(channel->base  + reg - 0x00, data);
    else if (reg < 0x0C)
        outb(channel->base  + reg - 0x06, data);
    else if (reg < 0x0E)
        outb(channel->ctrl  + reg - 0x0A, data);
    else if (reg < 0x16)
        outb(channel->bmide + reg - 0x0E, data);
    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, ATA_REG_CONTROL, channel->nIEN);
}

uint8_t ide_read(struct ide_channel* channel, uint8_t reg) {
    uint8_t result;
    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, ATA_REG_CONTROL, 0x80 | channel->nIEN);
    if (reg < 0x08)
        result = inb(channel->base + reg - 0x00);
    else if (reg < 0x0C)
        result = inb(channel->base  + reg - 0x06);
    else if (reg < 0x0E)
        result = inb(channel->ctrl  + reg - 0x0A);
    else if (reg < 0x16)
        result = inb(channel->bmide + reg - 0x0E);
    if (reg > 0x07 && reg < 0x0C)
        ide_write(channel, ATA_REG_CONTROL, channel->nIEN);
    return result;
}

void ide_read_buffer(struct ide_channel* channel, uint8_t reg, uint32_t buffer, uint32_t quads) {
    unsigned short port;

    if (reg < 0x08)
        port = channel->base + reg;
    else if (reg < 0x0C)
        port = channel->base + reg - 6;
    else if (reg < 0x0E)
        port = channel->ctrl + reg - 0x0A;
    else if (reg < 0x16)
        port = channel->bmide + reg - 0x0E;

    unsigned int *dest = (unsigned int *)buffer;
    __asm__ volatile (
        "pushw %%es\n\t"             // Save ES
        "movw %%ds, %%ax\n\t"        // DS -> AX
        "movw %%ax, %%es\n\t"        // AX -> ES
        "cld\n\t"                     // Clear direction flag for forward copying
        "1:\n\t"
        "insl\n\t"                    // Read DWORD from port to ES:DI
        "loop 1b\n\t"                 // Loop quads times
        "popw %%es\n\t"               // Restore ES
        :
        : "d"(port), "D"(dest), "c"(quads)  // port -> DX, dest -> DI, quads -> CX
        : "eax", "memory"
    );


}

static inline void outsw(uint16_t port, const void* addr, uint32_t count) {
    asm volatile (
        "cld; rep outsw"
        : "+S"(addr), "+c"(count)
        : "d"(port)
        : "memory"
    );
}

static inline void insw(uint16_t port, void* addr, uint32_t count) {
    asm volatile (
        "cld; rep insw"
        : "+D"(addr), "+c"(count)
        : "d"(port)
        : "memory"
    );
}


