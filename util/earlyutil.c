#include "earlyutil.h"
#include "../interrupts/idt.h"
#include <stdint.h>

int cursor = 0;

volatile char* BIOS_FRAMEBUFFER = (char*)0xB8000;

void BIOSclear() {
    for (int i = 0; i<80*25*2; i++) {
        BIOS_FRAMEBUFFER[i] = 0;
    }
    cursor = 0;
}

void BIOSprint(const char* message, char format) {
    int i = 0;
    while (message[i] != '\0') {
        if (message[i] == '\n') {
            cursor += 80 - (cursor % 80);
        } else {
            BIOS_FRAMEBUFFER[cursor*2] = message[i];
            BIOS_FRAMEBUFFER[cursor*2+1] = format;
            cursor++;
        }
        i++;
    }
}

uint8_t inb(uint16_t port) {
    uint8_t result;
    __asm__ volatile ("inb %1, %0" : "=a" (result) : "Nd" (port));
    return result;
}

void outb(uint16_t port, uint8_t data) {
    __asm__ volatile ("outb %0, %1" : : "a" (data), "Nd" (port));
}

uint32_t inl(uint16_t port) {
    uint32_t result;
    __asm__ volatile ("in %1, %0" : "=a" (result) : "Nd" (port));
    return result;
}

void outl(uint16_t port, uint32_t data) {
    __asm__ volatile ("out %0, %1" : : "a" (data), "Nd" (port));
}

void panic(const char* message) {
    asm volatile("cli");
    BIOSprint(message, 0xC);
    asm volatile("hlt");
    while (1) {}
}

uint64_t udivmod64(uint64_t numerator, uint64_t denominator, uint64_t *remainder) {
    if (denominator == 0) {
        if (remainder) *remainder = 0;
        return 0;
    }

    uint64_t q = 0;
    uint64_t r = 0;

    for (int i = 63; i >= 0; i--) {
        r <<= 1;
        r |= (numerator >> i) & 1;
        if (r >= denominator) {
            r -= denominator;
            q |= (1ULL << i);
        }
    }

    if (remainder) *remainder = r;
    return q;
}


void wait(uint32_t ms) {
    uint32_t start = ticks;
    while ((uint32_t)(ticks - start) < ms) {
        __asm__("hlt");
    }
}

