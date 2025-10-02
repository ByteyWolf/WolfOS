/* int86.h — wrapper types for the assembly int86 routine */

#ifndef INT86_H
#define INT86_H

#include <stdint.h>

struct int86regs {
    uint32_t edi, esi, ebp, esp;
    uint32_t ebx, edx, ecx, eax;
    uint16_t flags;
    uint16_t es, ds, fs, gs;
} __attribute__ ((packed));

extern void int86(int inum, struct int86regs *regs);

#endif
