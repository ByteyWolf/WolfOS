#ifndef EARLYUTIL_H
#define EARLYUTIL_H

#include <stdint.h>

#define VGA_COLOR(fg, bg) (( (bg) << 4) | (fg))

void BIOSprint(char* message, char format);
void panic(const char* message);
void BIOSclear(void);
uint8_t inb(uint16_t port);
void outb(uint16_t port, uint8_t data);
uint64_t udivmod64(uint64_t numerator, uint64_t denominator, uint64_t *remainder);
void outl(uint16_t port, uint32_t data);
uint32_t inl(uint16_t port);
void wait(uint32_t ms);
void qemuearlyprint(char* str);

#endif
