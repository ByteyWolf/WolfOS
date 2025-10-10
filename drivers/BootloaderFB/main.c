#include <stdint.h>
#include <stddef.h>
#include "definitions.h"
#include "../../boot/bootscheme.h"

extern void print(char* str);
extern void print_ptr(const void *p);
void print_uint32(uint32_t val, unsigned base, const char *digits);

extern void *memcpy(void *dst, const void *src, size_t n);
extern void *memset(void *dst,int value,size_t n);
extern void *memmove(void *dst, const void *src, uint32_t n);

extern boot_data_t* get_boot_data();

boot_data_t* multiboothdr;
framebuffer_t fb;

uint16_t __driver_class_priority=100;
char* __driver_name="Bootloader Framebuffer Driver";
char* __driver_author="ByteyWolf";


int init() {
    print("ByteyWolf BootFramebuffer Driver version 1.0\n");
    multiboothdr = get_boot_data();
    
    print("FB resolution: ");
    print_uint32(multiboothdr->framebuffer_width,10,"0123456789");
    print("x");
    print_uint32(multiboothdr->framebuffer_height,10,"0123456789");
    print("\n");
    if ((multiboothdr->framebuffer_width) < 160) {
        print("\xFCNo suitable framebuffer found...\xF7\n"); return 1;
    } else {
        print("oki!\n");
    }
    if (multiboothdr->framebuffer_addr >= 0xFFFF0000) {
        print("\xFCBootloader framebuffer mapped too high...\xF7\n"); return 1;
    }
    fb.addr = (uint32_t*)multiboothdr->framebuffer_addr;
    fb.bpp = multiboothdr->framebuffer_bpp;
    fb.height = multiboothdr->framebuffer_height;
    fb.width = multiboothdr->framebuffer_width;
    fb.pitch = multiboothdr->framebuffer_pitch;

    return 0;
}

uint32_t kgetpixel(uint32_t x, uint32_t y) {
    if (fb.width == 0 || fb.height == 0) return 0;
    if (x >= fb.width || y >= fb.height) return 0;

    uint8_t *row = (uint8_t*)fb.addr + y * fb.pitch;
    uint32_t *pixel = (uint32_t*)(row + x * (fb.bpp / 8));

    return *pixel; // returns ARGB/XRGB as stored in framebuffer
}


void ksetpixel(uint32_t x, uint32_t y, uint32_t color) {
    if (fb.width == 0 || fb.height == 0) return;
    if (x >= fb.width || y >= fb.height) return;

    uint8_t *row = (uint8_t*)fb.addr + y * fb.pitch;
    uint32_t *pixel = (uint32_t*)(row + x * (fb.bpp / 8));

    *pixel = color;
}

void kvshift(uint32_t rows) {
    if (rows == 0 || fb.width == 0 || fb.height == 0) return;

    if (rows >= fb.height) {
        memset(fb.addr, 0, fb.height * fb.pitch);
        return;
    }

    uint8_t *base = (uint8_t*)fb.addr;
    size_t move_bytes = (fb.height - rows) * fb.pitch;
    size_t clear_bytes = rows * fb.pitch;

    memmove(base, base + rows * fb.pitch, move_bytes);
    memset(base + move_bytes, 0, clear_bytes);
}


struct kfbmode getfbmode(void) {
    struct kfbmode result = { (uint16_t)multiboothdr->framebuffer_width, (uint16_t)multiboothdr->framebuffer_height };
    return result;
}
