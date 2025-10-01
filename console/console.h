#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdint.h>
#include <stdarg.h>
#include "../drivers/drivermodel.h"

#define VGA_COLOR_SWITCHER_BASE 240
#define VGA_GRAY 7

#define MIN_CAPACITY 4096
#define FONT_SIZE_X 8
#define FONT_SIZE_Y 16

#define CONSOLE_FLAG_TEXT   (1u << 0)
#define CONSOLE_FLAG_GRAPH  (1u << 1)

static const uint32_t vgamap[16] = {
    0x000000, // 0 = Black
    0x0000AA, // 1 = Blue
    0x00AA00, // 2 = Green
    0x00AAAA, // 3 = Cyan
    0xAA0000, // 4 = Red
    0xAA00AA, // 5 = Magenta
    0xAA5500, // 6 = Brown / Yellowish
    0xAAAAAA, // 7 = Light Gray
    0x555555, // 8 = Dark Gray
    0x5555FF, // 9 = Light Blue
    0x55FF55, // 10 = Light Green
    0x55FFFF, // 11 = Light Cyan
    0xFF5555, // 12 = Light Red
    0xFF55FF, // 13 = Light Magenta
    0xFFFF55, // 14 = Yellow
    0xFFFFFF  // 15 = White
};

struct console_entry {
    struct console_entry* next;
    struct console_entry* prev;
    uint16_t charcount;
    char data[MIN_CAPACITY];    
};

struct console {
    uint32_t flags;
    uint32_t charcount;
    uint32_t entries;

    struct console_entry* first;
    struct console_entry* last;
};

struct kfbmode {
    uint16_t width;
    uint16_t height;
} __attribute__((packed));


typedef struct kfbmode (*getkfbmode)(void);
typedef uint32_t (*kgetpixel_fn)(uint32_t x, uint32_t y);
typedef void (*ksetpixel_fn)(uint32_t x, uint32_t y, uint32_t argb);
typedef void (*kvshift_fn)(uint32_t rows);


struct console* init_console(uint32_t flags);
void switch_console(struct console* console);
uint8_t kswitch_graphics_driver(struct device_driver* driver);
void putchar(char chr);
void print(char* str);
void hexdump(char* ptr, uint32_t count);
void printf(const char *fmt, ...);

void print_ptr(const void *p);
void print_uint64(uint64_t val, unsigned base, const char *digits);
void print_uint32(uint32_t val, unsigned base, const char *digits);
void hexdump(char* ptr, uint32_t count);

#endif
