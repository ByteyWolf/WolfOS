#include <stdint.h>
#include "kalloc.h"
#include "../console/console.h"

struct memory_block {
    uint32_t size;
    uint8_t used; // 0 - free, 1 - used, 2 - unusable, 3 - required by allocator
    struct memory_block *next;
};

// note: the allocation table can grow but never shrink
// although, this should be fine since 1 million allocations will still be about ~32mb
// and since we repurpose old allocations it should be quite alright.
// if we have 1 million current allocations that's quite troublesome in the first place :p

static struct memory_block *allocator_map = 0;
static struct memory_block *free_blocks = 0;
void* next_memblock_addr = 0;
uint32_t page_depletion = 0;

uint32_t free_memory = 0;
uint32_t claimed_memory = 0;
uint32_t total_memory = 0;
uint32_t allocations = 0;

uint32_t memory_start = 0;

uint32_t get_free_mem_bytes() {
    return free_memory;
};
uint32_t get_total_mem_bytes() {
    return total_memory;
};

struct memory_block* mkblock() {
    struct memory_block *newblock = (struct memory_block*)free_blocks;
    if (free_blocks == 0) {
        if (page_depletion == MEMBLOCKS_PER_PAGE) { page_depletion = 0; next_memblock_addr = (void*)kmalloc(4096); }
        newblock = (struct memory_block*)next_memblock_addr;
        next_memblock_addr += sizeof(struct memory_block);
        page_depletion++;
    } else {
        free_blocks = newblock->next; // pull block out of free blocks
    }
    return newblock;
}

void cut_block(struct memory_block *prev_block, struct memory_block *block, uint32_t bytes_free_left, uint32_t bytes_used, uint32_t bytes_free_right, uint8_t marktype) {
    if (bytes_free_right != 0) {
        struct memory_block* newblock = mkblock();
        newblock->size = bytes_free_right;
        newblock->used = 0;
        newblock->next = block->next;
        block->next = newblock;
    }
    if (bytes_free_left != 0) {
        struct memory_block* newblock = mkblock();
        newblock->size = bytes_free_left;
        newblock->used = 0;
        newblock->next = block;
        prev_block->next = newblock;
    }
    block->size = bytes_used;
    block->used = marktype;
}

void free_block(struct memory_block *block) {
    block->used = 0;
    // merge any free blocks after
    struct memory_block *nextblk = block->next;
    while (1) {
        if (nextblk == 0 || nextblk->used == 1) { block->next = nextblk; break; }
        block->size += nextblk->size;

        // throw out nextblk
        struct memory_block *nextnextblk = nextblk->next;
        nextblk->next = free_blocks;
        free_blocks = nextblk;

        nextblk = nextnextblk;
    }
}

void* ksmalloc(uint32_t nbytes, uint32_t align, uint8_t markas) {
    struct memory_block *crt_block = allocator_map;
    struct memory_block *prev_block = 0;
    struct memory_block *last_free_block = 0;
    uint32_t block_start = 0;
    while (crt_block != 0) {
        if (crt_block->used == 0) {
            if (last_free_block != 0) {
                // nuke current block from orbit !!
                // printf("\xFA" "Housekeeping done: merged %u %u\xF7\n", last_free_block->size, crt_block->size);
                last_free_block->next = crt_block->next;
                last_free_block->size += crt_block->size;
                block_start -= crt_block->size;
                crt_block->next = free_blocks;
                free_blocks = crt_block;
                crt_block = last_free_block;
            }
            if (prev_block == 0 || prev_block->used == 1) {
                last_free_block = crt_block;
            }
        } else {
            last_free_block = 0;
        }

        uint32_t offset_required = (align == 0 || block_start % align == 0) ? 0 : align - (block_start % align);
       
        if (crt_block->used == 0 && crt_block->size >= nbytes + offset_required) {
            cut_block(prev_block, crt_block, offset_required, nbytes, (crt_block->size - nbytes - offset_required), markas);
            allocations++;
            free_memory -= nbytes;
            claimed_memory += nbytes;
            if (markas == 1) {
                for (uint32_t i = 0; i < nbytes; i++) {
                    ((char*)(memory_start + offset_required + block_start))[i] = 0;
                }
            }
            return (void*)(memory_start + offset_required + block_start);
        }
        block_start += crt_block->size;
        prev_block = crt_block;
        crt_block = crt_block->next;
    }
    return 0;
}

void kmarkunusable(uint32_t nbytestotal, uint32_t cut_from) {
    struct memory_block *crt_block = allocator_map;
    struct memory_block *prev_block = 0;
    uint32_t block_start = 0;
    uint32_t nbytesremaining = nbytestotal;
    uint32_t cut_to = cut_from + nbytestotal;
    while (crt_block != 0) {     
        uint32_t block_end = block_start + crt_block->size;  
        if (block_end >= cut_from) {
            if (crt_block->used == 0) {
                uint32_t offsetl = cut_from <= block_start ? cut_from - block_start : 0;
                uint32_t offsetr = cut_to >= block_end ? block_end : block_end - cut_to;
                cut_block(prev_block, crt_block, offsetl, crt_block->size - offsetl - offsetr, offsetr, 2);
            }
            nbytesremaining = nbytesremaining <= crt_block->size ? 0 : nbytesremaining - crt_block->size;
            if (nbytesremaining == 0) {
                return;
            }
        }
        block_start += crt_block->size;
        prev_block = crt_block;
        crt_block = crt_block->next;
    }
}

void* kmalloc(uint32_t nbytes) {
    return ksmalloc(nbytes, 0, 1);
}

void* kamalloc(uint32_t nbytes, uint32_t align) {
    return ksmalloc(nbytes, align, 1);
}

void init_kallocator(uint32_t min_usable_location, uint32_t max_usable_location) {    
    memory_start = min_usable_location;
    next_memblock_addr = (void*)min_usable_location;
    allocator_map = (struct memory_block*)next_memblock_addr;
    next_memblock_addr += sizeof(struct memory_block);
    page_depletion++;
    printf("allocated allocator map at %p\n", allocator_map);
    allocator_map->size = max_usable_location - min_usable_location;
    allocator_map->used = 0;
    allocator_map->next = 0;
    free_memory = max_usable_location - min_usable_location;
    total_memory = free_memory;
    ksmalloc(4096, 0, 3);
}

uint8_t kfree(void* ptr) {
    uint32_t lookfor = (uint32_t)(ptr) - memory_start;
    struct memory_block *crt_block = allocator_map;
    uint32_t block_start = 0;
    while (crt_block != 0) {       
        if (crt_block->used == 1 && lookfor == block_start) {
            free_memory -= crt_block->size;
            claimed_memory += crt_block->size;
            free_block(crt_block);
            allocations--;

            return 1;
        }
        block_start += crt_block->size;
        crt_block = crt_block->next;
    }
    printf("\xFC" "Failed to find segment to free: %u!\xF7\n", lookfor);
    return 0;
}

void debugprintkmap() {
    printf("\xFE[ Memory Map: %p ]\xF7\n", allocator_map);
    struct memory_block *crt_block = allocator_map;
    uint32_t block_start = 0;
    while (crt_block != 0) {       
        printf("\xF3used %u size %u  ---  %p\xF7\n", (uint32_t)(crt_block->used), crt_block->size, (char*)allocator_map+block_start);
        block_start += crt_block->size;
        crt_block = crt_block->next;
    }
    printf("\xFE%u allocation(s), %u KB free\xF7\n", allocations, free_memory/1024);
}
