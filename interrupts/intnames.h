// intnames.h
#pragma once

// Convenience: unified array
static const char *intnames[48] = {
    "Divide Error", "Debug Exception", "NMI", "Breakpoint",
    "Overflow", "BOUND Range Exceeded", "Invalid Opcode",
    "Device Not Available", "Double Fault", "Coprocessor Segment Overrun",
    "Invalid TSS", "Segment Not Present", "Stack-Segment Fault",
    "General Protection Fault", "Page Fault", "Reserved",
    "x87 FPU Floating-Point Error", "Alignment Check", "Machine Check",
    "SIMD Floating-Point Exception", "Virtualization Exception", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved", "Security Exception", "Reserved",
    "IRQ0: Timer", "IRQ1: Keyboard", "IRQ2: Cascade", "IRQ3: COM2/4",
    "IRQ4: COM1/3", "IRQ5: LPT2 / Sound Card", "IRQ6: Floppy Disk",
    "IRQ7: LPT1 / Parallel Port", "IRQ8: RTC", "IRQ9: ACPI / IRQ2",
    "IRQ10: SCSI / NIC", "IRQ11: SCSI / NIC", "IRQ12: PS/2 Mouse",
    "IRQ13: FPU", "IRQ14: Primary ATA", "IRQ15: Secondary ATA"
};
