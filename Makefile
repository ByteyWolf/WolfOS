# --- Driver configuration ---
DRIVERS_DIR := drivers

# --- Existing parent Makefile configuration ---
KERNEL     := kernel.elf
ISO        := wolfos.iso
BUILD_DIR  := build
ISO_DIR    := iso
GRUB_DIR   := $(ISO_DIR)/boot/grub

ASFLAGS := -m32 -ffreestanding
CFLAGS  := -m32 -ffreestanding -Wall -Wextra -nostdlib -fno-builtin -fno-stack-protector -funsigned-char -O0
LDFLAGS := -T linker.ld -m elf_i386

SOURCES  := start.S main.c util/earlyutil.c util/gdt.c util/interrupts/idt.c util/strings.c console/console.c mm/mem.c mm/kalloc.c progman/elf.c drivers/pci.c drivers/pci_names.c drivers/drivermodel.c util/cpuid.c util/int86.S
OBJECTS := $(SOURCES:%.c=$(BUILD_DIR)/%.o)
OBJECTS := $(OBJECTS:%.S=$(BUILD_DIR)/%.o)

# --- Targets ---

all: drivers $(ISO)

# Build all drivers
drivers:
	$(MAKE) -C $(DRIVERS_DIR); \

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	gcc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.S
	mkdir -p $(dir $@)
	gcc $(ASFLAGS) -c $< -o $@

$(KERNEL): $(OBJECTS)
	ld $(LDFLAGS) -o $@ $^

$(GRUB_DIR)/grub.cfg:
	mkdir -p $(GRUB_DIR)
	echo 'set timeout=0'                >  $@
	echo 'set default=0'                >> $@
	echo ''                              >> $@
	echo 'menuentry "Wolf Kernel" {'    >> $@
	echo '    multiboot /boot/$(KERNEL)'>> $@
	echo '    boot'                     >> $@
	echo '}'                            >> $@

$(ISO): $(KERNEL) $(GRUB_DIR)/grub.cfg
	cp $(KERNEL) $(ISO_DIR)/boot/$(KERNEL)
	grub-mkrescue --locales= --fonts= --themes= -o $(ISO) $(ISO_DIR) >/dev/null 2>&1

run: all $(ISO)
	qemu-system-i386 \
	-cdrom $(ISO) \
	-m 8M \
	-machine q35 \
	-vga std \
	-drive id=disk,file=qemu/sata_disk.img,format=raw,if=none \
	-device ahci,id=ahci \
	-device ide-hd,drive=disk,bus=ahci.0 \
	-debugcon stdio


clean:
	rm -rf $(BUILD_DIR) $(KERNEL) $(ISO_DIR)/boot/$(KERNEL) $(ISO)
	$(MAKE) -C $(DRIVERS_DIR) clean; \

.PHONY: all run clean drivers
