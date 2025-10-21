# WolfOS
> a *work-in-progress* **32-bit x86 operating system** designed for low-end hardware.

I made this operating system with the goal of both learning OSDev and C/asm. As I am new to both, some of my practices are not the best. Bear with me ...

## Features
- GDT and interrupt management
- Color console
- Working physical memory allocator (malloc and free) respecting the memory map
- Basic ELF loader
- Preliminary driver model for dynamic component loading
- BIOS framebuffer via GRUB and high-res text support
## What I'm working on
- IDE drivers 🐾
- Paging and virtual memory
- First userland app

## Building and running
On Linux:
*GCC and grub-mkrescue must be available*
- Run `make` in the project directory to build *wolfos.iso*
- Test with QEMU by running `make run`
