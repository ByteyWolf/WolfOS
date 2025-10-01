#ifndef GDT_H
#define GDT_H

#include <stdint.h>

#define GDT_FLAGS      0x0C
#define GDT_ACCESS_KERNEL_CODE 0x9A
#define GDT_ACCESS_KERNEL_DATA 0x92
#define GDT_ACCESS_USER_CODE   0xFA
#define GDT_ACCESS_USER_DATA   0xF2

struct __attribute__((packed)) GDTEntry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  flags_limit_high;
    uint8_t  base_high;
};

struct __attribute__((packed)) GDTR {
    uint16_t limit;
    uint32_t base;
};

struct __attribute__((packed)) FlatGDT {
    struct GDTEntry null;
    struct GDTEntry codeKernel;
    struct GDTEntry dataKernel;
    struct GDTEntry codeUser;
    struct GDTEntry dataUser;
    struct GDTEntry code16;
    struct GDTEntry data16;
};

struct GDTEntry make_gdt_entry(uint32_t base, uint32_t limit, uint8_t access, uint8_t flags);
void load_gdt(void);

#endif
