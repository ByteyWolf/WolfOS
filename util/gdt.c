#include "gdt.h"
#include <stdint.h>

struct GDTEntry make_gdt_entry(uint32_t base, uint32_t limit, uint8_t access, uint8_t flags) {
    struct GDTEntry e;
    e.limit_low = (uint16_t)(limit & 0xFFFF);
    e.base_low  = (uint16_t)(base & 0xFFFF);
    e.base_mid  = (uint8_t)((base >> 16) & 0xFF);
    e.access    = access;
    e.flags_limit_high = (uint8_t)(((limit >> 16) & 0x0F) | ((flags & 0x0F) << 4));
    e.base_high = (uint8_t)((base >> 24) & 0xFF);
    return e;
}

void load_gdt(void) {
    static struct FlatGDT gdt;

    gdt.null       = make_gdt_entry(0, 0, 0, 0);
    gdt.codeKernel = make_gdt_entry(0, 0xFFFFF, GDT_ACCESS_KERNEL_CODE, GDT_FLAGS);
    gdt.dataKernel = make_gdt_entry(0, 0xFFFFF, GDT_ACCESS_KERNEL_DATA, GDT_FLAGS);
    gdt.codeUser   = make_gdt_entry(0, 0xFFFFF, GDT_ACCESS_USER_CODE, GDT_FLAGS);
    gdt.dataUser   = make_gdt_entry(0, 0xFFFFF, GDT_ACCESS_USER_DATA, GDT_FLAGS);
    gdt.code16 = make_gdt_entry(0, 0xFFFF, 0x9A, 0x0); // 16-bit code segment
    gdt.data16 = make_gdt_entry(0, 0xFFFF, 0x92, 0x0);

    struct GDTR gdtr;
    gdtr.limit = sizeof(gdt) - 1;
    gdtr.base  = (uint32_t)&gdt;

    asm volatile(
        "lgdt %0\n\t"
        "ljmp $0x08, $1f\n\t"
        "1:\n\t"
        "mov $0x10, %%ax\n\t"
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "mov %%ax, %%fs\n\t"
        "mov %%ax, %%gs\n\t"
        "mov %%ax, %%ss\n\t"
        : 
        : "m"(gdtr)
        : "ax", "memory"
    );
}
