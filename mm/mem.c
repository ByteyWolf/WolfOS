#include "../console/console.h"
#include "../util/multiboot.h"
#include "../util/earlyutil.h"
#include "mem.h"
#include "kalloc.h"

static inline uint32_t floor_page(uint32_t addr) { return addr & ~0xFFFu; }
static inline uint32_t ceil_page(uint32_t addr)  { return (addr + 0xFFFu) & ~0xFFFu; }

void init_memory_manager(multiboot_info_t* mbd, void* _kernel_start, void* _kernel_end) {
    uint32_t kernel_start = (uint32_t)_kernel_start;
    uint32_t kernel_end   = (uint32_t)_kernel_end;

    if (!(mbd->flags & (1 << 6))) panic("invalid memory map given by GRUB bootloader");

    uint32_t best_start = 0;
    uint32_t best_size = 0;

    uint32_t map_addr = (uint32_t)mbd->mmap_addr;
    uint32_t map_end  = map_addr + mbd->mmap_length;
    uint32_t p = map_addr;

    while (p < map_end) {
        multiboot_memory_map_t* e = (multiboot_memory_map_t*)p;
        uint32_t entry_size = e->size;
        uint32_t next_p = p + entry_size + sizeof(e->size);

        uint64_t base = ((uint64_t)e->addr_high << 32) | e->addr_low;
        uint64_t len  = ((uint64_t)e->len_high  << 32) | e->len_low;

        if (e->type == MULTIBOOT_MEMORY_AVAILABLE && len > 0) {
            if (base < 0x100000000ULL) {
                uint32_t start = (uint32_t)base;
                uint64_t end64 = base + len;
                if (end64 > 0x100000000ULL) end64 = 0x100000000ULL;
                uint32_t end = (uint32_t)end64;

                if (start < kernel_end && end > kernel_end) {
                    uint32_t overlap = kernel_end - start;
                    start += overlap;
                    if (len > overlap) {
                        end = (uint32_t)(base + len);
                        if (end > 0xFFFFFFFFu) end = 0xFFFFFFFFu;
                    } else {
                        start = end = 0;
                    }
                } else if (end <= kernel_end) {
                    start = end = 0;
                }

                if (start < end) {
                    start = ceil_page(start);
                    end   = floor_page(end);
                    if (end > start) {
                        uint32_t sz = end - start;
                        if (sz > best_size) { best_size = sz; best_start = start; }
                    }
                }
            }
        }

        if (next_p <= p) break;
        p = next_p;
    }

    if (best_start == 0 || best_size == 0) panic("no usable memory region found for allocator");

    uint32_t alloc_start = best_start;
    uint32_t alloc_end   = best_start + best_size;

    init_kallocator(alloc_start, alloc_end);

    p = map_addr;
    while (p < map_end) {
        multiboot_memory_map_t* e = (multiboot_memory_map_t*)p;
        uint32_t entry_size = e->size;
        uint32_t next_p = p + entry_size + sizeof(e->size);

        uint64_t base = ((uint64_t)e->addr_high << 32) | e->addr_low;
        uint64_t len  = ((uint64_t)e->len_high  << 32) | e->len_low;

        if (len > 0 && base < 0x100000000ULL) {
            uint32_t start = (uint32_t)base;
            uint64_t end64 = base + len;
            if (end64 > 0x100000000ULL) end64 = 0x100000000ULL;
            uint32_t end = (uint32_t)end64;

            if (end > start) {
                uint32_t mark_start = (start < alloc_start) ? alloc_start : start;
                uint32_t mark_end   = (end   > alloc_end)   ? alloc_end   : end;
                if (mark_end > mark_start && e->type != MULTIBOOT_MEMORY_AVAILABLE) {
                    kmarkunusable(mark_end - mark_start, mark_start);
                }
            }
        }

        if (next_p <= p) break;
        p = next_p;
    }
}
