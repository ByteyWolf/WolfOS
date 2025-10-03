# drivers/Makefile
# List drivers here (space-separated)
DRIVERS ?= USB_EHCI VGA BootloaderFB

# Tools & flags (tweak to match your build)
CC       := gcc
LD       := ld
XXD      := xxd

CFLAGS   := -m32 -ffreestanding -Wall -Wextra -nostdlib -fno-builtin \
            -fno-stack-protector -funsigned-char -fPIC -c
LDFLAGS  := -m elf_i386 -shared -nostdlib --build-id=none --gc-sections -s -T driver.ld

# --- internal rule generator ---
# For each driver DIR we compile DIR/*.c -> DIR/*.o, link to DIR/driver.so, xxd -> DIR/driver_elf.h

# Build a list of per-driver targets like HelloWorldDriver/driver.so
SO_TARGETS := $(foreach d,$(DRIVERS),$(d)/driver.so)

all: $(SO_TARGETS)

# generic object rule: compile any .c to .o in-place
# (this works for files in subdirectories like HelloWorldDriver/foo.c -> HelloWorldDriver/foo.o)
%.o: %.c
	@echo "  CC $<"
	$(CC) $(CFLAGS) $< -o $@

# Per-driver linking & xxd generation
# uses the driver's .o files discovered by wildcard
define DRIVER_template
$(1)_SRCS := $(wildcard $(1)/*.c)
$(1)_OBJS := $$(patsubst %.c,%.o,$$($(1)_SRCS))

$(1)/driver.so: $$($(1)_OBJS)
	@echo "  LD $$@"
	$(LD) $(LDFLAGS) $$^ -o $$@
	@echo "  XXD -> $(1)/driver_elf.h"
	$(XXD) -i $$@ > $(1)/driver_elf.h
endef

# instantiate rules for each driver in DRIVERS
$(foreach drv,$(DRIVERS),$(eval $(call DRIVER_template,$(drv))))

.PHONY: clean
clean:
	@echo "Cleaning driver builds..."
	$(foreach d,$(DRIVERS), rm -f $(d)/*.o $(d)/driver.so $(d)/driver_elf.h 2>/dev/null || true; )
