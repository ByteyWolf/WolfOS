#include "util/earlyutil.h"
#include "util/strings.h"
#include "util/gdt.h"
#include "util/interrupts/idt.h"
#include "util/multiboot.h"
#include "util/cpuid.h"
#include "util/int86.h"

#include "console/console.h"

#include "mm/mem.h"
#include "mm/kalloc.h"

#include "progman/elf.h"

#include "drivers/USB_EHCI/driver_elf.h"
#include "drivers/VGA/driver_elf.h"
#include "drivers/drivermodel.h"

#include <stdint.h>

extern char _kernel_start;
extern char _kernel_end;

void kernel_main(multiboot_info_t* mbd, unsigned int magic) {
    if(magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        panic("invalid bootloader magic number!");
    }

    BIOSclear();
    load_gdt();
    load_cpuid();
    if (cpu_has_feature(CPUID_FEAT_EDX_SSE)) {enable_sse();}

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

    struct int86_regs regs = {0};
    regs.ax=0;
    print("calling\n");
    int86(0x16, &regs);
    print("back\n");

    
    //register_driver(VESA_VBE_driver_so, VESA_VBE_driver_so_len);
    struct device_driver* vga_generic = register_driver(VGA_driver_so, VGA_driver_so_len);
    kswitch_graphics_driver(vga_generic);
    register_driver(USB_EHCI_driver_so, USB_EHCI_driver_so_len);

    printf("\xFEInitializing devices...\xF7\n");

    init_devices();
    
    while (1) {}
}
