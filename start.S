.section .multiboot
.align 4
.long 0x1BADB002
.long 0x0
.long -(0x1BADB002 + 0x0)

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
