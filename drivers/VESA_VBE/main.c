#include <stdint.h>
#include <stddef.h>
#include "definitions.h"

extern uint8_t inb(uint16_t port);
extern void outb(uint16_t port, uint8_t value);
extern void wait(uint32_t ms);
extern void print(char* str);
extern void print_ptr(const void *p);

extern void *memcpy(void *dst, const void *src, size_t n);
extern void *memset(void *dst,int value,size_t n);
extern void *memmove(void *dst, const void *src, uint32_t n);

extern void* kmalloc(uint32_t nbytes);
extern void* kamalloc(uint32_t nbytes, uint32_t align);
extern uint8_t kfree(void* ptr);
extern void int86(int inum, struct int86_regs *regs);

extern char _lowbuf;

struct kfbmode {
    uint16_t width;
    uint16_t height;
} __attribute__((packed));

uint16_t __driver_class_priority=100;
char* __driver_name="VESA Driver";
char* __driver_author="ByteyWolf";


int init() {
    print("ByteyWolf VESA Driver version 1.0\n");
    // get vbe info
    struct int86_regs* regs = kmalloc(sizeof(struct int86_regs));
    regs->ax=0x4f00;
    regs->es=seg(&_lowbuf);
    regs->di=off(&_lowbuf);
    print("calling\n");
    int86(0x10, regs);
    print("back\n");
    if (regs->ax!=0x004f) {print("uh oh\n"); return 1;};
    struct VbeInfoBlock* vbeinfo = (struct VbeInfoBlock*)_lowbuf;
    print(vbeinfo->VbeSignature);

    return 0;
}