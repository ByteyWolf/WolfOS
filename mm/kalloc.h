#ifndef KALLOC_H
#define KALLOC_H

#include "mem.h"
#include <stdint.h>

#define MEMBLOCKS_PER_PAGE 32

void* kmalloc(uint32_t nbytes);
void* kamalloc(uint32_t nbytes, uint32_t align);
void kmarkunusable(uint32_t nbytestotal, uint32_t cut_from);
uint8_t kfree(void* ptr);

void init_kallocator(uint32_t min_usable_location, uint32_t max_usable_location);
void debugprintkmap();

uint32_t get_free_mem_bytes();
uint32_t get_total_mem_bytes();

#endif
