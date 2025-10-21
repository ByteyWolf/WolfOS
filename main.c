#include "util/earlyutil.h"
#include "util/strings.h"
#include "util/gdt.h"
#include "boot/bootscheme.h"
#include "boot/multiboot2.h"
#include "util/cpuid.h"

#include "interrupts/idt.h"

#include "console/console.h"

#include "mm/mem.h"
#include "mm/kalloc.h"

#include "progman/elf.h"

#include "drivers/USB_EHCI/driver_elf.h"
#include "drivers/BootloaderFB/driver_elf.h"
#include "drivers/drivermodel.h"

#include <stdint.h>

extern char _kernel_start;
extern char _kernel_end;

const char* videodrv_base[] = {BootloaderFB_driver_so};
const uint8_t videodrv_base_c = 1;

void kernel_main(uint32_t magic, struct multiboot_info* mbd_e) {
    boot_data_t* mbd = init_boot(magic, mbd_e);

    //BIOSclear();
    load_gdt();
    load_cpuid();

    init_memory_manager(mbd, &_kernel_start, &_kernel_end); // We can now use kmalloc(), kamalloc() and free()!
    struct console* console = init_console(CONSOLE_FLAG_TEXT);
    switch_console(console); // We can now use printf()!

    printf("\xFA" "Free memory: %u KiB (%u B)\xF7\n\n", get_free_mem_bytes()/1024, get_free_mem_bytes());
    printf("CPU Vendor: %s\n", get_cpu_vendor());
    printf("SSE/MMX support: %s/%s\n", cpu_has_feature(CPUID_FEAT_EDX_SSE) ? "yes" : "no", cpu_has_feature(CPUID_FEAT_EDX_MMX) ? "yes" : "no");

    print("Loading IDT...\n");
    init_idt_and_keyboard();
    print("Enabling interrupts...\n");
    asm volatile("sti");
    print("\xFAInterrupts enabled\xF7\n");

    init_progman(mbd);
    
    register_driver(USB_EHCI_driver_so, USB_EHCI_driver_so_len);
    for (uint8_t drvidx = 0; drvidx < videodrv_base_c; drvidx++) {
        char* videodrv_so = videodrv_base[drvidx];
        struct device_driver* videodrv = register_driver(videodrv_so, 0xFFFFFFFF);
        printf("this drv result: %u\n", videodrv->init_result);
        if (videodrv->init_result == 0) {
            kswitch_graphics_driver(videodrv);
            break;
        }
    }
    
    printf("\xFEInitializing devices...\xF7\n");

    init_devices();

    printf("\xFB" "Boot source: %s\n", mbd->boot_source == BOOTSOURCE_BIOS ? "Legacy" : "UEFI");
    
    while (1) {}
}
