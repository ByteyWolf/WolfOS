#ifndef ELF_H
#define ELF_H

#include <stdint.h>

enum {
    R_386_NONE     = 0,
    R_386_32       = 1,
    R_386_PC32     = 2,
    R_386_GOT32    = 3,
    R_386_PLT32    = 4,
    R_386_COPY     = 5,
    R_386_GLOB_DAT = 6,
    R_386_JMP_SLOT = 7,
    R_386_RELATIVE = 8
};

struct e_ident {
    char header[4];
    char bitness;
    char endianness;
    char version;
    char os_abi;
    char os_abi_version;
    char padding[7];
};

struct elf_file {
    struct e_ident ident;
    uint16_t type;
    uint16_t machine;
    uint32_t version_big;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
};

struct e_section {
    uint32_t sh_name;
    uint32_t sh_type;
    uint32_t sh_flags;
    uint32_t sh_addr;
    uint32_t sh_offset;
    uint32_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint32_t sh_addralign;
    uint32_t sh_entsize;
};

struct rel_entry {
    uint32_t r_offset;
    uint32_t r_info;
};

struct sym_entry {
    uint32_t st_name;
    uint32_t st_value;
    uint32_t st_size;
    unsigned char st_info;
    unsigned char st_other;
    uint16_t st_shndx;
};

struct driver_init_passport {
    char* loaded_location;
    char* dynsym;
    char* dynstr;
    uint32_t sym_count;
};

typedef int (*driver_init_v1)(void);
typedef int (*driver_bind_pci)(uint8_t bus, uint8_t slot, uint8_t func);

#define ELF32_R_SYM(i) ((i) >> 8)
#define ELF32_R_TYPE(i) ((uint8_t)(i))

struct driver_init_passport* load_driver_bin(char* addr, uint32_t size);
void* get_elf_symbol(struct driver_init_passport* passport, char* symbol_name);

#endif
