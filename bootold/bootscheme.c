#include "multiboot2.h"
#include "bootscheme.h"
#include "../util/earlyutil.h"
#include "../mm/mem.h"
#include "../console/console.h"
#include <stdint.h>

boot_data_t* init_boot(uint32_t multiboot_magic, struct multiboot_info* mbdata) {
    if (multiboot_magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        panic("Bootloader gave broken multiboot");
    }

    static boot_data_t bootdata;
    bootdata.framebuffer_addr = 0;
    bootdata.mmap_entries = 0;
    
    uint32_t total_bytes = mbdata->total_size;
    uint8_t* crtPtr = (uint8_t*)mbdata + 8;  // Skip header (total_size + reserved)
    uint8_t* endPtr = (uint8_t*)mbdata + total_bytes;
    
    while (crtPtr < endPtr) {
        uint32_t tagType = *(uint32_t*)crtPtr;
        uint32_t tagSize = *(uint32_t*)(crtPtr + 4);
        
        if (tagType == 0 && tagSize == 8) break;
        
        switch (tagType) {
            case MULTIBOOT_TAG_TYPE_MMAP: {
                uint32_t entry_size = *(uint32_t*)(crtPtr + 8);
                uint32_t entry_version = *(uint32_t*)(crtPtr + 12);
                uint32_t entries = (tagSize - 16) / entry_size;
                
                bootdata.mmap_base = (void*)(crtPtr + 16);
                bootdata.mmap_entries = entries;
                break;
            }
            
            case MULTIBOOT_TAG_TYPE_FRAMEBUFFER: {
                uint64_t fbAddr = *(uint64_t*)(crtPtr + 8);
                uint32_t fbPitch = *(uint32_t*)(crtPtr + 16);
                uint32_t fbWidth = *(uint32_t*)(crtPtr + 20);
                uint32_t fbHeight = *(uint32_t*)(crtPtr + 24);
                uint8_t fbBpp = *(uint8_t*)(crtPtr + 28);
                uint8_t fbType = *(uint8_t*)(crtPtr + 29);
                
                if (fbType != MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED) {
                    bootdata.framebuffer_addr = fbAddr;
                    bootdata.framebuffer_pitch = fbPitch;
                    bootdata.framebuffer_width = fbWidth;
                    bootdata.framebuffer_height = fbHeight;
                    bootdata.framebuffer_bpp = fbBpp;
                }
                break;
            }
            
            default:
                qemuearlyprint("who knows\n");
                break;
        }
        
        printf("tag type %u size %ub\n", tagType, tagSize);
        
        uint32_t alignedSize = (tagSize + 7) & ~7;
        crtPtr += alignedSize;
    }
    return &bootdata;
}
