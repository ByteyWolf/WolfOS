.section .multiboot2
.align 8

multiboot_header_start:
    .long 0xE85250D6
    .long 0
    .long multiboot_header_end - multiboot_header_start
    .long -(0xE85250D6 + 0 + (multiboot_header_end - multiboot_header_start))

    /* Framebuffer tag */
    .align 8
    .short 5
    .short 0
    .long 20
    .long 1920
    .long 1080
    .long 32

    /* End tag - required */
    .align 8
    .short 0
    .short 0
    .long 8
multiboot_header_end:

.section .text
.global _start
.align 16
_start:
    cli
    mov $stack_top, %esp
    push %ebx                        /* Multiboot2 info structure pointer */
    push %eax                        /* Multiboot2 magic value */
    call kernel_main

.section .bss
.align 16
stack_bottom:
    .skip 0x32000
stack_top:
