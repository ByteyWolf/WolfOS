.section .multiboot
.align 4

/* Multiboot1 header */
.equ MULTIBOOT_MAGIC, 0x1BADB002
/* PAGE_ALIGN (1) | MEM_INFO (2) | VIDEO_MODE (4) */
.equ MULTIBOOT_FLAGS, 0x00000007
.equ MULTIBOOT_CHECKSUM, -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS) /* 32-bit wrap */

.long MULTIBOOT_MAGIC
.long MULTIBOOT_FLAGS
.long MULTIBOOT_CHECKSUM

/* a.out kludge (5 longs) - safe to include/unused by most loaders */
.long 0
.long 0
.long 0
.long 0
.long 0

/* Video mode request fields (present if bit 2 set in flags) */
/* mode_type: 0 = linear graphics, 1 = text. */
/* width, height: pixels (0 == no preference) */
/* depth: bits per pixel (e.g. 32) */
.long 0          /* mode_type: 0 = linear framebuffer */
.long 1920       /* preferred width - set to 0 if you want "no preference" */
.long 1080       /* preferred height */
.long 32         /* preferred bits-per-pixel */

.section .text
.global _start
.align 16
_start:
    cli
    mov $stack_top, %esp
    push %eax
    push %ebx
    call kernel_main

.section .bss
.align 16
stack_bottom:
    .skip 0x32000
stack_top:
