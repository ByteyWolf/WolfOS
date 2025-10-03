#include <stdint.h>
#include "elf.h"
#include "../console/console.h"
#include "../util/earlyutil.h"
#include "../util/strings.h"
#include "../util/int86.h"
#include "../drivers/pci.h"
#include "../util/multiboot.h"

#include "../mm/kalloc.h"

#define FAIL_IF(cond, reason) \
    do { \
        if (cond) { \
            printf("\xFC" "ELF load fail: " reason "\xF7\n"); \
            return 0; \
        } \
    } while (0)

struct e_section* get_section_data(struct elf_file* binary, uint32_t index) {
    return (struct e_section*)((char*)binary + binary->e_shoff + index * binary->e_shentsize);
}

multiboot_info_t* multiboothdr;
multiboot_info_t* get_multiboot() {
    return multiboothdr;
}

void* resolve_symbol(char* symname) {
    if (strcmp(symname, "print")==0) return print;
    if (strcmp(symname, "print_uint32")==0) return print_uint32;
    if (strcmp(symname, "print_uint64")==0) return print_uint64;
    if (strcmp(symname, "print_ptr")==0) return print_ptr;
    if (strcmp(symname, "putchar")==0) return putchar;
    if (strcmp(symname, "pci_read_word")==0) return pci_read_word;
    if (strcmp(symname, "pci_read_byte")==0) return pci_read_byte;
    if (strcmp(symname, "pci_read_dword")==0) return pci_read_dword;
    if (strcmp(symname, "kmalloc")==0) return kmalloc;
    if (strcmp(symname, "kamalloc")==0) return kamalloc;
    if (strcmp(symname, "kfree")==0) return kfree;
    if (strcmp(symname, "pci_write_word")==0) return pci_write_word;
    if (strcmp(symname, "pci_write_dword")==0) return pci_write_dword;
    if (strcmp(symname, "wait")==0) return wait;
    if (strcmp(symname, "outb")==0) return outb;
    if (strcmp(symname, "inb")==0) return inb;
    if (strcmp(symname, "memcpy")==0) return memcpy;
    if (strcmp(symname, "memset")==0) return memset;
    if (strcmp(symname, "memmove")==0) return memmove;
    if (strcmp(symname, "int86")==0) return int86;
    if (strcmp(symname, "get_multiboot")==0) return get_multiboot; 
    return 0;
}

void init_progman(multiboot_info_t* mbh) {
    multiboothdr = mbh;
}

uint32_t process_relocations(char* image, char* loadedimg, struct e_section* rel_dyn, struct e_section* dynsym_sec, struct e_section* dynstr_sec) {
    char* dynstr = image+dynstr_sec->sh_offset;
    uint32_t count = rel_dyn->sh_size / sizeof(struct rel_entry);
    struct rel_entry* rels = (struct rel_entry*)(image + rel_dyn->sh_offset);

    for (uint32_t i = 0; i < count; i++) {
        uint32_t offset = rels[i].r_offset;
        uint32_t sym_index = ELF32_R_SYM(rels[i].r_info);
        uint32_t type = ELF32_R_TYPE(rels[i].r_info);

        struct sym_entry* sym = (struct sym_entry*)(image + dynsym_sec->sh_offset + sym_index * sizeof(struct sym_entry));
        const char* name = dynstr + sym->st_name;

        // resolve the symbol
        //void* addr = resolve_symbol(name);  // your kernel function address map
        void* addr;
        if (sym->st_shndx != 0) {
            struct e_section* sec = get_section_data((struct elf_file*)image, sym->st_shndx);
            addr = loadedimg + sym->st_value;
        } else {
            addr = resolve_symbol(name);
        }
        
        //printf("(%u/%u) Resolve: %s (type %u) -> %p\n", i+1, count, name, type, addr);
        if (!addr && type != R_386_RELATIVE) {
            printf("FAIL Resolve Missing Addr: %s (type %u) -> %p\n", name, type, addr);
            return 1;
        }

        if (type == R_386_GLOB_DAT || type == R_386_JMP_SLOT) {
            *(uint32_t*)(loadedimg + offset) = (uint32_t)addr;
        } else if (type == R_386_RELATIVE) { 
            uint32_t addend = *(uint32_t*)(loadedimg + offset);
            *(uint32_t*)(loadedimg + offset) = (uint32_t)loadedimg + addend;
        } else {
            printf("FAIL Resolve: %s (type %u) -> %p\n", name, type, addr);
            return 1;
        }
    }
    return 0;
}

uint32_t get_elf_min_csize(struct elf_file* binary) {
    uint32_t minsize = 0;
    for (uint32_t section_idx = 0; section_idx < binary->e_shnum; section_idx++) {
        struct e_section* section = get_section_data(binary, section_idx);
        if ((section->sh_flags & 2) == 2 && minsize < section->sh_addr + section->sh_size) minsize = section->sh_addr + section->sh_size;
    }
    return minsize;
}

void* get_elf_symbol(struct driver_init_passport* passport, char* symbol_name) {
    if (!passport) {printf("\xFC" "Invalid passport passed to get_elf_symbol\xF7\n"); return 0;};

    struct sym_entry* syms = (struct sym_entry*)(passport->dynsym);
    char* strtab = passport->dynstr;
    uint32_t sym_count = passport->sym_count;

    for (uint32_t i = 0; i < sym_count; i++) {
        const char* name = strtab + syms[i].st_name;
        if (strcmp(name, symbol_name) == 0) {
            return (char*)(passport->loaded_location) + syms[i].st_value;
        }
    }
    return 0;
}

struct driver_init_passport* load_driver_bin(char* addr, uint32_t size) {
    struct elf_file* binary = (struct elf_file*)addr;

    FAIL_IF(binary->ident.header[0] != 0x7F || binary->ident.header[1] != 0x45 || binary->ident.header[2] != 0x4c || binary->ident.header[3] != 0x46, "Not ELF file");
    FAIL_IF(binary->ident.bitness != 1, "Bitness mismatch");
    FAIL_IF(binary->ident.endianness != 1, "Endianness mismatch");
    FAIL_IF(binary->ident.version != 1, "Version mismatch");
    FAIL_IF(binary->machine != 3, "Architecture mismatch");
    FAIL_IF(binary->version_big != 1, "Subversion mismatch");

    uint32_t minalloc = get_elf_min_csize(binary);
    char* loadloc = (char*)kamalloc(minalloc, 4098);
    FAIL_IF(loadloc == 0, "Insufficient system memory to load this driver");
    memset(loadloc, 0, minalloc);
    //printf("\xFEPlacing ELF file at: %p (%u bytes)\n", loadloc, minalloc);

    char* shstrtab = (char*)binary + (get_section_data(binary, binary->e_shstrndx)->sh_offset);
    struct e_section* rel_dyn = 0;
    struct e_section* dynsym = 0;
    struct e_section* dynstr = 0;

    //hexdump(loadloc, minalloc);
    //return 0;
    //for (uint32_t i = 0; i<minalloc; i++) {
    //    loadloc[i]=0;
    //}
    //printf("%p %x\n", loadloc, minalloc);
    //debugprintkmap();
    //return 0;
    

    for (uint32_t section_idx = 0; section_idx < binary->e_shnum; section_idx++) {
        struct e_section* section = get_section_data(binary, section_idx);
        char* sh_name = shstrtab + section->sh_name;

        //printf("\xFE%s\xF7\n", sh_name);
        if (rel_dyn == 0 && strcmp(".rel.dyn", sh_name) == 0) rel_dyn = section;
        if (dynsym == 0 && strcmp(".dynsym", sh_name) == 0) dynsym = section;
        if (dynstr == 0 && strcmp(".dynstr", sh_name) == 0) dynstr = section;
        if (section->sh_flags & 8) continue;
        char* dest = loadloc + section->sh_addr;
        char* src = (char*)binary + section->sh_offset;
        uint32_t size = section->sh_size;

        if ((section->sh_flags & 2)) {
            //printf("\xF7%p[%u] --> %p\n", src, size, dest);
            for (uint32_t i = 0; i<size; i++) {
                dest[i] = src[i];
            }
        }
    }
    uint32_t reloc_fails = process_relocations((char*)binary, loadloc, rel_dyn, dynsym, dynstr);
    FAIL_IF(reloc_fails != 0, "Not all relocations succeeded");

    struct driver_init_passport* passport = kmalloc(sizeof(struct driver_init_passport));
    passport->loaded_location = loadloc;
    passport->dynsym = loadloc+(dynsym->sh_addr);
    passport->dynstr = loadloc+(dynstr->sh_addr);
    passport->sym_count = dynsym->sh_size / sizeof(struct sym_entry);

    return passport;
}
