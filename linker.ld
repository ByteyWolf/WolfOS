ENTRY(_start)

SECTIONS
{
    _kernel_start = .;
    . = 1M;

    .text : {
        *(.multiboot2)
        *(.text*)
    }

    .rodata : {
        *(.rodata*)
    }

    .data : {
        *(.data*)
    }

    .bss : {
        *(.bss*)
        *(COMMON)
    }

    _kernel_end = .;
}
