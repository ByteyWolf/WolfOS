#ifndef IDT_H
#define IDT_H

#include <stdint.h>

#define GENERATE_STUB(label, target, int_no) \
extern void label(); asm(".global " #label "\n" #label ":\n" "pusha\n" "push %ds\n" "push %es\n" "mov $0x10, %ax\n" "mov %ax, %ds\n" "mov %ax, %es\n" "push $" #int_no "\n" "call " #target "\n" "add $4, %esp\n" "pop %es\n" "pop %ds\n" "popa\n" "iret\n");

struct __attribute__((packed)) IDTEntry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  zero;
    uint8_t  type_attr;
    uint16_t offset_high;
};

struct __attribute__((packed)) IDTR {
    uint16_t limit;
    uint32_t base;
};

void set_idt_entry(int n, void (*handler)(), uint16_t sel, uint8_t type_attr);
void init_idt_and_keyboard();
void keyboard_handler_c();

extern volatile uint64_t ticks;

#endif
