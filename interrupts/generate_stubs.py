#!/usr/bin/env python3

# Config
cpu_exceptions = 32          # 0-31
hardware_irqs = 16           # IRQ0-15
total_ints = 256

# Handlers
cpu_handler = "generic_handler"
irq_handlers = ["timer_handler", "keyboard_handler"] + ["generic_handler"] * (hardware_irqs-2)
unused_handler = "generic_handler"

print("// Auto-generated ISR stubs")
print("#include \"idt.h\"")
print()

# CPU exceptions 0-31
for i in range(cpu_exceptions):
    print(f"GENERATE_STUB(isr{i}_stub, {cpu_handler}, {i})")

# Hardware IRQs (32-47)
for i in range(hardware_irqs):
    irq_no = 32 + i
    handler = irq_handlers[i]
    print(f"GENERATE_STUB(irq{i}_stub, {handler}, {irq_no})")

# Unused interrupts 48-255
for i in range(48, total_ints):
    print(f"GENERATE_STUB(isr{i}_stub, {unused_handler}, {i})")

print()
print("// IDT assignment")
print("void init_idt_entries(void) {")
for i in range(total_ints):
    if i < cpu_exceptions:
        print(f"    set_idt_entry({i}, isr{i}_stub, 0x08, 0x8E);")
    elif 32 <= i < 32+hardware_irqs:
        irq_idx = i-32
        print(f"    set_idt_entry({i}, irq{irq_idx}_stub, 0x08, 0x8E);")
    else:
        print(f"    set_idt_entry({i}, isr{i}_stub, 0x08, 0x8E);")
print("}")
