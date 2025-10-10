#ifndef BOOTSCHEME_H
#define BOOTSCHEME_H

#include <stdint.h>
#include "multiboot2.h"

#define MEMORY_AVAILABLE              1
#define MEMORY_RESERVED               2
#define MEMORY_ACPI_RECLAIMABLE       3
#define MEMORY_NVS                    4
#define MEMORY_BADRAM                 5

struct mmap_entry
{
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
    uint32_t reserved;
} __attribute__((packed));
typedef struct mmap_entry memory_map_t;

struct boot_data {
    uint64_t header_start;
    uint64_t header_size;
    uint32_t mmap_entries;
    memory_map_t* mmap_base;

    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t framebuffer_bpp;
};
typedef struct boot_data boot_data_t;

struct multiboot_info {
    uint32_t total_size;
    uint32_t reserved;
};

boot_data_t* init_boot(uint32_t multiboot_magic, struct multiboot_info* mbdata);

#endif