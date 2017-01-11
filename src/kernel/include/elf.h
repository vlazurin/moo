#ifndef H_ELF
#define H_ELF

#include <stdint.h>

typedef struct elf_header
{
    uint8_t magic;
    uint8_t signature[3];
    uint8_t arch;
    uint8_t endian_type;
    uint8_t elf_version;
    uint8_t os_abi;
    uint8_t reserverd[8];
    uint16_t type;
    uint16_t instruction_set;
    uint32_t elf_version_2;
    uint32_t entry_point;
    uint32_t header_table_position;
    uint32_t section_table_position;
    uint32_t flags;
    uint16_t header_size;
    uint16_t program_header_entry_size;
    uint16_t program_header_entries_count;
    uint16_t section_header_entry_size;
    uint16_t section_header_entries_count;
    uint16_t string_table;
}__attribute__((packed)) elf_header_t;

typedef struct elf_relocation
{
    uint32_t offset;
    uint32_t info;
} __attribute__((packed)) elf_relocation_t;

typedef struct elf_section
{
    uint32_t name;
    uint32_t type;
    uint32_t flags;
    uint32_t address;
    uint32_t offset;
    uint32_t size;
    uint32_t link;
    uint32_t info;
    uint32_t address_align;
    uint32_t entry_size;
} __attribute__((packed)) elf_section_t;

typedef struct elf_symbol
{
    uint32_t name;
    uint32_t value;
    uint32_t size;
    uint8_t info;
    uint8_t other;
    uint16_t shndx;
} __attribute__((packed)) elf_symbol_t;

int load_elf(char* filename, void **entry_point);

#endif
