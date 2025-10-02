// idt.c
#include <stdint.h>
#include "../util/earlyutil.h"
#include "../util/gdt.h"
#include "idt.h"
#include "../console/console.h"
#include "intnames.h"
#include "idtstubs.h"
#include "../util/int86.h"

static struct IDTEntry idt[256];
volatile uint64_t ticks = 0;

void set_idt_entry(int n, void (*handler)(), uint16_t sel, uint8_t type_attr) {
    uint32_t addr = (uint32_t)handler;
    idt[n].offset_low = (uint16_t)(addr & 0xFFFF);
    idt[n].selector = sel;
    idt[n].zero = 0;
    idt[n].type_attr = type_attr;
    idt[n].offset_high = (uint16_t)((addr >> 16) & 0xFFFF);
}

void pic_remap() {
    int offset1 = 0x20;
    int offset2 = 0x28;
    uint8_t a1 = inb(0x21);
    uint8_t a2 = inb(0xA1);
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, offset1);
    outb(0xA1, offset2);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);
    (void)a1; (void)a2;
}

void pic_restore(uint8_t master_mask, uint8_t slave_mask) {
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x08);
    outb(0xA1, 0x70);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, master_mask);
    outb(0xA1, slave_mask);
}


void pit_set_frequency(uint32_t freq) {
    uint16_t divisor = 1193182 / freq;
    outb(0x43, 0x36);
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);
}


static inline void load_idt(void) {
    struct IDTR idtr;
    idtr.limit = sizeof(idt) - 1;
    idtr.base  = (uint32_t)&idt;
    asm volatile("lidt %0" :: "m"(idtr) : "memory");
}

void keyboard_handler(uint8_t interruptNum) {
    uint8_t scancode = inb(0x60);
    printf("Keyboard interrupt: %u\n", (uint32_t)scancode);
   
    outb(0x20, 0x20);
    outb(0xA0, 0x20);
}

void timer_handler(uint8_t interruptNum) {
    ticks++;
    outb(0x20, 0x20);
    outb(0xA0, 0x20);
}

void generic_handler(uint8_t interruptNum) {
    printf("\n\xFC[ GENERIC INTERRUPT ]\n\xFBName:\xFF %s\n\xFB" "Code:\xFF %u\xF7\n", intnames[interruptNum], (uint32_t)interruptNum);
    if (interruptNum < 32) {
        asm volatile("hlt");
        while (1) {};
    }
    outb(0x20, 0x20);
    outb(0xA0, 0x20);
}

void init_idt_and_keyboard() {
    pic_remap();
    pit_set_frequency(1000);
    

    init_idt_entries();
    load_idt();
    outb(0x21, 0xFC);
    outb(0xA1, 0xFF);
}
