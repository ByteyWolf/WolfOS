/* int86.h — wrapper types for the assembly int86 routine */

#ifndef INT86_H
#define INT86_H

#include <stdint.h>

struct int86_regs {
    uint16_t ax;
    uint16_t bx;
    uint16_t cx;
    uint16_t dx;
    uint16_t si;
    uint16_t di;
    uint16_t bp;
    uint16_t ds;
    uint16_t es;
    uint16_t flags;
} __attribute__((packed));


void int86(int inum, struct int86_regs *regs);

#endif
