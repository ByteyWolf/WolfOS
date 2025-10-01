#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include "console.h"
#include "../mm/kalloc.h"
#include "../util/strings.h"
#include "../util/earlyutil.h"
#include "../drivers/drivermodel.h"
#include "../progman/elf.h"

#include "font.h"

struct console* active_console = 0;
uint8_t graphics_text_mode = 1;
volatile char* bios_buf = (char*)0xb8000;
uint16_t cursor_x = 0;
uint16_t cursor_y = 0;
uint16_t capacity_x = 80;
uint16_t capacity_y = 25;
uint8_t draw_mode = VGA_GRAY;

kgetpixel_fn kgetpixel = 0;
ksetpixel_fn ksetpixel = 0;
kvshift_fn kvshift = 0;

struct console_entry* add_entry(struct console* console) {
    struct console_entry* entry = kmalloc(sizeof(struct console_entry));
    if (entry == 0) return 0;

    if (console->last != 0) {
        entry->prev = console->last;
        console->last->next = entry;
    } else {
        console->first = entry;
        entry->prev = 0;
    }
    console->last = entry;
    entry->charcount = 0;
    entry->next = 0;
    memset(entry->data, 0, MIN_CAPACITY);
    console->entries++;
    return entry;
}

// in the future these will call out to the responsible graphics driver if not in bios text mode
void gr_shiftup(uint16_t rows) {
    if (graphics_text_mode) {
        memcpy(bios_buf, bios_buf+(160*rows), 80*(50-(rows*2)));
        memset((void*)(bios_buf + 80*(50-(rows*2))), 0, rows * 160);
    } else {
        kvshift(rows*FONT_SIZE_Y);
    }
}

void gr_drawchar(char chr) {
    if (graphics_text_mode) {
        bios_buf[cursor_y * 160 + cursor_x * 2] = chr;
        bios_buf[cursor_y * 160 + cursor_x * 2 + 1] = draw_mode;
    } else {
        uint16_t topX = cursor_x * FONT_SIZE_X;
        uint16_t topY = cursor_y * FONT_SIZE_Y;
        uint8_t *glyph = &consolefont_default[4 + chr * FONT_SIZE_Y];
        for (uint16_t row = 0; row < FONT_SIZE_Y; row++) {
            uint8_t rowbits = glyph[row]; // 8 bits for 8 pixels in this row
            for (uint16_t col = 0; col < FONT_SIZE_X; col++) {
                // Only 8 bits are valid per row; if FONT_SIZE_X > 8, you may need multiple bytes
                if (rowbits & (1 << (7 - col))) {
                    ksetpixel(topX + col, topY + row, vgamap[draw_mode]);
                } else {
                    ksetpixel(topX + col, topY + row, 0);
                }
            }
        }


    }
}

void gr_renderchar(char chr) {
    if (chr == '\n' || cursor_x >= capacity_x) {
        if (cursor_y >= capacity_y - 1) {
            gr_shiftup(1);
        } else {
            cursor_y++;
        }
        cursor_x = 0;
        return;
    }
    // note: we compile with unsigned chars!
    if (chr >= VGA_COLOR_SWITCHER_BASE) {
        draw_mode = chr - VGA_COLOR_SWITCHER_BASE;
    } else {
        gr_drawchar(chr);
        cursor_x++;
    }

}

uint8_t flushline(char *crtline) {
    uint16_t maxhere = cursor_x;
    for (cursor_x = 0; cursor_x < maxhere; cursor_x++) {
        char c = crtline[maxhere-cursor_x];
        if (c == '\n') {
            cursor_x--;
            maxhere--;
            continue;
        }
        if (c >= VGA_COLOR_SWITCHER_BASE) {
            draw_mode = c - VGA_COLOR_SWITCHER_BASE;
            cursor_x--;
            maxhere--;
            continue;
        }
        gr_drawchar(c);
    }
    if (cursor_y == 0) return 1;
    cursor_y--;
    cursor_x = 0;
    return 0;
}

void redraw() {
    if (!active_console) return;
    if (graphics_text_mode) return;
    // just begin from the end
    cursor_y = capacity_y - 1; //uint16_t
    cursor_x = 0;
    struct console_entry* crtblock = active_console->last;
    char crtline[capacity_x];
    while (crtblock != 0 && cursor_y != 0xFFFF) {
        for (int chrid = crtblock->charcount - 1; chrid >= 0; chrid--) {
            char chr = crtblock->data[chrid];
            crtline[cursor_x] = chr;
            if (chr == '\n' || cursor_x >= capacity_x) {
                if (flushline(crtline)) break;
            }
            cursor_x++;
        }
        crtblock = crtblock->prev;
    }
    cursor_x--;
    flushline(crtline);
    cursor_x = 0;
    cursor_y = capacity_y - 1;
}

void putchar(char chr) {
    if (!active_console) return;
    outb(0xe9, chr);

    struct console_entry* tail = active_console->last;
    if (!tail) {
        tail = add_entry(active_console);
        if (!tail) return;
    }

    if (tail->charcount >= MIN_CAPACITY - 1) {
        struct console_entry* newtail = add_entry(active_console);
        if (!newtail) return;
        tail = newtail;
    }

    tail->data[tail->charcount++] = chr;
    active_console->charcount++;
    gr_renderchar(chr);
}

struct console* init_console(uint32_t flags) {
    struct console* console = kmalloc(sizeof(struct console));
    console->flags = flags;
    console->charcount = 0;
    console->entries = 0;
    console->first = 0;
    console->last = 0;
    return console;
}

void switch_console(struct console* console) {
    // TODO: redraw everything
    active_console = console;
}

uint8_t kswitch_graphics_driver(struct device_driver* driver) {
    struct driver_init_passport* passport = driver->passport;
    getkfbmode symbol = (getkfbmode)get_elf_symbol(passport, "getfbmode");
    kgetpixel_fn kgetpixel_tmp = (kgetpixel_fn)get_elf_symbol(passport, "kgetpixel");
    ksetpixel_fn ksetpixel_tmp = (ksetpixel_fn)get_elf_symbol(passport, "ksetpixel");
    kvshift_fn kvshift_tmp = (kvshift_fn)get_elf_symbol(passport, "kvshift");
    if (symbol == 0 || kgetpixel_tmp == 0 || ksetpixel_tmp == 0 || kvshift_tmp == 0) {
        printf("not a graphics driver\n");
        return 0;
    }
    kgetpixel = kgetpixel_tmp;
    ksetpixel = ksetpixel_tmp;
    kvshift = kvshift_tmp;
    struct kfbmode fbmode = symbol();
    capacity_x = fbmode.width / FONT_SIZE_X;
    capacity_y = fbmode.height / FONT_SIZE_Y;
    printf("%dx%d\n", fbmode.width, fbmode.height);
    graphics_text_mode = 0;
    redraw();
    return 1;
}

/////// stupid garbage below

void print(char* str) {
    while (*str != 0) { putchar(*str); str++; }
}

void hexdump(char* ptr, uint32_t count) {
    for (uint32_t i = 0; i<count; i++) {
        printf("%x ", ptr[i]);
    }
    putchar('\n');
}

/* -------- 32-bit number printing (no heap, no helpers) -------- */
void print_uint32(uint32_t val, unsigned base, const char *digits) {
    if (val >= base) print_uint32(val / base, base, digits);
    putchar(digits[val % base]);
}

void print_signed32(int32_t v) {
    if (v < 0) {
        putchar('-');
        print_uint32((uint32_t)(-v), 10, "0123456789");
    } else {
        print_uint32((uint32_t)v, 10, "0123456789");
    }
}

/* -------- 64-bit number printing via helper -------- */
void print_uint64(uint64_t val, unsigned base, const char *digits) {
    if (val == 0) { putchar('0'); return; }
    uint64_t q = val;
    uint64_t r;
    if (q >= base) {
        q = udivmod64(q, base, &r);
        print_uint64(q, base, digits);
        putchar(digits[r]);
    } else {
        putchar(digits[q]);
    }
}

void print_signed64(int64_t v) {
    if (v < 0) {
        putchar('-');
        uint64_t uv = (uint64_t)(-(v + 1)) + 1ULL;
        print_uint64(uv, 10, "0123456789");
    } else {
        print_uint64((uint64_t)v, 10, "0123456789");
    }
}

void print_ptr(const void *p) {
    uintptr_t v = (uintptr_t)p;
    putchar('0'); putchar('x');
    #if UINTPTR_MAX > 0xFFFFFFFF
    print_uint64((uint64_t)v, 16, "0123456789abcdef");
    #else
    print_uint32((uint32_t)v, 16, "0123456789abcdef");
    #endif
}

/* -------- hybrid printf -------- */
void printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    for (const char *p = fmt; *p; p++) {
        if (*p != '%') { putchar(*p); continue; }

        p++;
        if (!*p) break;

        switch (*p) {
            case 'c': putchar((char)va_arg(ap,int)); break;
            case 's': {
                const char *s = va_arg(ap,const char*);
                if (!s) s = "(null)";
                while (*s) putchar(*s++);
                break;
            }
            case 'd': print_signed32(va_arg(ap,int)); break;
            case 'u': print_uint32(va_arg(ap,unsigned int),10,"0123456789"); break;
            case 'x': print_uint32(va_arg(ap,unsigned int),16,"0123456789abcdef"); break;
            case 'p': print_ptr(va_arg(ap,void*)); break;
            case 'l': /* check for long long: %lld / %llu / %lx */
                p++;
                switch(*p) {
                    case 'd': print_signed64(va_arg(ap,long long)); break;
                    case 'u': print_uint64(va_arg(ap,unsigned long long),10,"0123456789"); break;
                    case 'x': print_uint64(va_arg(ap,unsigned long long),16,"0123456789abcdef"); break;
                    default: putchar('%'); putchar('l'); putchar(*p); break;
                }
                break;
                    case '%': putchar('%'); break;
                    default: putchar('%'); putchar(*p); break;
        }
    }

    va_end(ap);
}