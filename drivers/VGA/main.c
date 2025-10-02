#include <stdint.h>
#include <stddef.h>

extern uint8_t inb(uint16_t port);
extern void outb(uint16_t port, uint8_t value);
extern void wait(uint32_t ms);
extern void print(char* str);

extern void *memcpy(void *dst, const void *src, size_t n);
extern void *memset(void *dst,int value,size_t n);
extern void *memmove(void *dst, const void *src, size_t n);

struct kfbmode {
    uint16_t width;
    uint16_t height;
} __attribute__((packed));

uint16_t __driver_class_priority = 100;
char* __driver_name = "VGA Driver";
char* __driver_author = "Silly Wolf";

#define VGA_AC_INDEX 0x3C0
#define VGA_AC_WRITE 0x3C0
#define VGA_AC_READ  0x3C1
#define VGA_MISC_WRITE 0x3C2
#define VGA_DAC_READ_INDEX 0x3C7
#define VGA_DAC_WRITE_INDEX 0x3C8
#define VGA_DAC_DATA 0x3C9
#define VGA_MISC_READ 0x3CC
#define VGA_SEQ_INDEX 0x3C4
#define VGA_SEQ_DATA  0x3C5
#define VGA_GC_INDEX  0x3CE
#define VGA_GC_DATA   0x3CF
#define VGA_CRTC_INDEX 0x3D4
#define VGA_CRTC_DATA  0x3D5
#define VGA_INSTAT_READ 0x3DA

#define VGA_NUM_SEQ_REGS 5
#define VGA_NUM_CRTC_REGS 25
#define VGA_NUM_GC_REGS 9
#define VGA_NUM_AC_REGS 21
#define VGA_NUM_REGS (1+VGA_NUM_SEQ_REGS+VGA_NUM_CRTC_REGS+VGA_NUM_GC_REGS+VGA_NUM_AC_REGS)

#define VGA_FB_PHYS_A000 0x000A0000U
#define VGA_FB_PHYS_B000 0x000B0000U
#define VGA_FB_PHYS_B800 0x000B8000U

unsigned char g_640x480x16[] = {
    0xE3,0x03,0x01,0x08,0x00,0x06,0x5F,0x4F,0x50,0x82,0x54,0x80,0x0B,0x3E,
    0x00,0x40,0x00,0x00,0x00,0x00,0x00,0x00,0xEA,0x0C,0xDF,0x28,0x00,0xE7,0x04,0xE3,0xFF,
    0x00,0x00,0x00,0x00,0x03,0x00,0x05,0x0F,0xFF,
    0x00,0x01,0x02,0x03,0x04,0x05,0x14,0x07,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,
    0x01,0x00,0x0F,0x00,0x00
};

unsigned char g_720x480x16[] = {
    0xE7,0x03,0x01,0x08,0x00,0x06,0x6B,0x59,0x5A,0x82,0x60,0x8D,0x0B,0x3E,
    0x00,0x40,0x06,0x07,0x00,0x00,0x00,0x00,0xEA,0x0C,0xDF,0x2D,0x08,0xE8,0x05,0xE3,0xFF,
    0x00,0x00,0x00,0x00,0x03,0x00,0x05,0x0F,0xFF,
    0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
    0x01,0x00,0x0F,0x00,0x00
};



void write_regs(unsigned char *regs){
    unsigned i;
    outb(VGA_MISC_WRITE,*regs++);
    for(i=0;i<VGA_NUM_SEQ_REGS;i++){ outb(VGA_SEQ_INDEX,i); outb(VGA_SEQ_DATA,*regs++); }
    outb(VGA_CRTC_INDEX,0x03);
    outb(VGA_CRTC_DATA,inb(VGA_CRTC_DATA)|0x80);
    outb(VGA_CRTC_INDEX,0x11);
    outb(VGA_CRTC_DATA,inb(VGA_CRTC_DATA)&~0x80);
    regs[0x03]|=0x80; regs[0x11]&=~0x80;
    for(i=0;i<VGA_NUM_CRTC_REGS;i++){ outb(VGA_CRTC_INDEX,i); outb(VGA_CRTC_DATA,*regs++); }
    for(i=0;i<VGA_NUM_GC_REGS;i++){ outb(VGA_GC_INDEX,i); outb(VGA_GC_DATA,*regs++); }
    for(i=0;i<VGA_NUM_AC_REGS;i++){ (void)inb(VGA_INSTAT_READ); outb(VGA_AC_INDEX,i); outb(VGA_AC_WRITE,*regs++); }
    (void)inb(VGA_INSTAT_READ); outb(VGA_AC_INDEX,0x20);
}

void read_regs(unsigned char *regs){
    unsigned i;
    *regs++=inb(VGA_MISC_READ);
    for(i=0;i<VGA_NUM_SEQ_REGS;i++){ outb(VGA_SEQ_INDEX,i); *regs++=inb(VGA_SEQ_DATA); }
    for(i=0;i<VGA_NUM_CRTC_REGS;i++){ outb(VGA_CRTC_INDEX,i); *regs++=inb(VGA_CRTC_DATA); }
    for(i=0;i<VGA_NUM_GC_REGS;i++){ outb(VGA_GC_INDEX,i); *regs++=inb(VGA_GC_DATA); }
    for(i=0;i<VGA_NUM_AC_REGS;i++){ (void)inb(VGA_INSTAT_READ); outb(VGA_AC_INDEX,i); *regs++=inb(VGA_AC_READ); }
    (void)inb(VGA_INSTAT_READ); outb(VGA_AC_INDEX,0x20);
}

static inline uintptr_t get_fb_base_phys(void){
    unsigned seg; outb(VGA_GC_INDEX,6); seg=inb(VGA_GC_DATA); seg >>= 2; seg &= 3;
    if(seg == 2) return VGA_FB_PHYS_B000;
    if(seg == 3) return VGA_FB_PHYS_B800;
    return VGA_FB_PHYS_A000;
}


static inline void set_plane(unsigned p){
    uint8_t pmask = (uint8_t)(1U << (p & 3));
    outb(VGA_GC_INDEX, 4); outb(VGA_GC_DATA, (uint8_t)(p & 3));   /* may be card-specific but kept as original */
    outb(VGA_SEQ_INDEX, 2); outb(VGA_SEQ_DATA, pmask);
}

static volatile uint8_t *g_fb = NULL;
static uintptr_t g_fb_phys_base = 0;
static inline void refresh_fb_ptr(void){
    g_fb_phys_base = get_fb_base_phys();
    g_fb = (volatile uint8_t*)(uintptr_t)g_fb_phys_base;
}

static inline uint8_t fb_peekb_cached(const uint8_t *fb, size_t off){ return fb[off]; }
static inline void fb_pokeb_cached(uint8_t *fb, size_t off, uint8_t v){ fb[off] = v; }
static inline void fb_memcpy_cached(uint8_t *dst, const void *src, size_t n){ memcpy(dst, src, n); }

static unsigned g_wd = 0, g_ht = 0;
static void (*g_write_pixel)(unsigned,unsigned,unsigned) = 0;

static void write_pixel4p(unsigned x, unsigned y, unsigned c){
    unsigned wd_in_bytes = g_wd >> 3;               /* faster than /8 */
    size_t off = (size_t)wd_in_bytes * (size_t)y + (size_t)(x >> 3);
    unsigned bit = 7u - (x & 7u);
    uint8_t mask = (uint8_t)(1u << bit);

    uint8_t *fb = (uint8_t*)(uintptr_t)g_fb;        /* non-volatile local pointer for inner ops */

    /* write per-plane — set_plane + RMW */
    unsigned pmask = 1u;
    for(unsigned p = 0; p < 4; ++p){
        set_plane(p);
        uint8_t cur = fb_peekb_cached(fb, off);
        if (pmask & c) fb_pokeb_cached(fb, off, (uint8_t)(cur | mask));
        else           fb_pokeb_cached(fb, off, (uint8_t)(cur & (uint8_t)~mask));
        pmask <<= 1;
    }
}


static void clear_screen(unsigned color){
    if (g_wd == 0 || g_ht == 0) return;
    unsigned bytes_per_row = g_wd >> 3;
    size_t plane_size = (size_t)bytes_per_row * (size_t)g_ht;
    uint8_t *fb = (uint8_t*)(uintptr_t)g_fb;
    for(unsigned p = 0; p < 4; ++p){
        set_plane(p);
        uint8_t fill = (color & (1u << p)) ? (uint8_t)0xFF : (uint8_t)0x00;
        /* use libc memset (extern) which may be optimized on the platform */
        memset((void*)fb, (int)fill, plane_size);
    }
}

static void set_mode(unsigned char *regs, unsigned wd, unsigned ht, void(*pixel_fn)(unsigned,unsigned,unsigned)){
    __asm__ volatile("cli");
    write_regs(regs);
    __asm__ volatile("sti");
    refresh_fb_ptr();
    g_wd = wd; g_ht = ht; g_write_pixel = pixel_fn;
}

static inline unsigned rgb_to_vga_index(uint32_t argb) {
    uint8_t r = (argb >> 16) & 0xFF;
    uint8_t g = (argb >> 8) & 0xFF;
    uint8_t b = (argb) & 0xFF;

    unsigned rbit = (r > 127) ? 1u : 0u;
    unsigned gbit = (g > 127) ? 1u : 0u;
    unsigned bbit = (b > 127) ? 1u : 0u;

    unsigned avg = ((unsigned)r + (unsigned)g + (unsigned)b) / 3u;
    unsigned bright = (avg > 170) ? 1u : 0u;

    return (unsigned)((bright << 3) | (rbit << 2) | (gbit << 1) | (bbit << 0)) & 0xF;
}

static inline uint32_t vga_index_to_argb(unsigned idx) {
    idx &= 0xF;
    unsigned bright = (idx & 8) ? 1u : 0u;
    uint8_t onval = bright ? 0xFF : 0x80;
    uint8_t r = (idx & 4) ? onval : 0x00;
    uint8_t g = (idx & 2) ? onval : 0x00;
    uint8_t b = (idx & 1) ? onval : 0x00;
    return (uint32_t)((r << 16) | (g << 8) | b);
}

uint32_t kgetpixel(uint32_t x, uint32_t y) {
    if (g_wd == 0 || g_ht == 0) return 0;
    if (x >= g_wd || y >= g_ht) return 0;

    unsigned wd_in_bytes = g_wd >> 3;
    size_t off = (size_t)wd_in_bytes * (size_t)y + (size_t)(x >> 3);
    unsigned bit = 7u - (x & 7u);
    uint8_t mask = (uint8_t)(1u << bit);

    unsigned idx = 0;
    uint8_t *fb = (uint8_t*)(uintptr_t)g_fb;

    for (unsigned p = 0; p < 4; ++p) {
        set_plane(p);
        uint8_t v = fb_peekb_cached(fb, off);
        if (v & mask) idx |= (1u << p);
    }
    return vga_index_to_argb(idx);
}

void ksetpixel(uint32_t x, uint32_t y, uint32_t argb) {
    if (g_wd == 0 || g_ht == 0) return;
    if (x >= g_wd || y >= g_ht) return;

    unsigned idx = rgb_to_vga_index(argb) & 0xF;

    if (g_write_pixel) {
        g_write_pixel(x, y, idx);
        return;
    }

    unsigned wd_in_bytes = g_wd >> 3;
    size_t off = (size_t)wd_in_bytes * (size_t)y + (size_t)(x >> 3);
    unsigned bit = 7u - (x & 7u);
    uint8_t mask = (uint8_t)(1u << bit);

    uint8_t *fb = (uint8_t*)(uintptr_t)g_fb;
    unsigned pmask = 1u;
    for(unsigned p = 0; p < 4; ++p){
        set_plane(p);
        uint8_t cur = fb_peekb_cached(fb, off);
        if (pmask & idx) fb_pokeb_cached(fb, off, (uint8_t)(cur | mask));
        else             fb_pokeb_cached(fb, off, (uint8_t)(cur & (uint8_t)~mask));
        pmask <<= 1;
    }
}


void kvshift(uint32_t rows) {
    if (rows == 0) return;
    if (g_wd == 0 || g_ht == 0) return;

    if (rows >= g_ht) {
        unsigned bytes_per_row = g_wd >> 3;
        size_t clear_bytes = (size_t)bytes_per_row * (size_t)g_ht;
        uint8_t *fb = (uint8_t*)(uintptr_t)g_fb;
        for (unsigned p = 0; p < 4; ++p) {
            set_plane(p);
            memset((void*)fb, 0, clear_bytes);
        }
        return;
    }

    unsigned bytes_per_row = g_wd >> 3;
    size_t src_offset_bytes = (size_t)rows * (size_t)bytes_per_row;
    size_t move_bytes = (size_t)(g_ht - rows) * (size_t)bytes_per_row;
    size_t clear_bytes = (size_t)rows * (size_t)bytes_per_row;
    uint8_t *fb = (uint8_t*)(uintptr_t)g_fb;

    for (unsigned p = 0; p < 4; ++p) {
        set_plane(p);
        memmove((void*)fb, (const void*)(fb + src_offset_bytes), move_bytes);
        memset((void*)(fb + move_bytes), 0, clear_bytes);
    }
}

struct kfbmode getfbmode(void) {
    struct kfbmode result = { (uint16_t)g_wd, (uint16_t)g_ht };
    return result;
}

int init(void) {
    set_mode(g_720x480x16, 720, 480, write_pixel4p);
    clear_screen(0);
    return 0;
}
