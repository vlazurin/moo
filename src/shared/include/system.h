#ifndef H_SYSTEM_GENERIC
#define H_SYSTEM_GENERIC

#include <stdint.h>

#define PAGE_ALIGN(n) (((n) + 0xfff) & 0xfffff000)

uint16_t crc16(uint8_t *data, uint32_t length);

#define GDT_SIZE 3

typedef struct gdt_entry
{
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__ ((packed)) gdt_entry_t;

typedef struct gdt_register
{
    uint16_t limit;
    uint32_t base;
} __attribute__ ((packed)) gdt_register_t;

void gdt_set_gate(uint16_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t granularity);
void gdt_init();
void set_gdt();

void hlt();

#define MEMORY_MAP_REGION_FREE 1
#define MEMORY_MAP_REGION_RESERVED 2

typedef struct memory_map_entry {
    uint32_t base_low;
    uint32_t base_high;
    uint32_t length_low;
    uint32_t length_high;
    uint32_t type;
    uint32_t ACPI;
} __attribute__((packed)) memory_map_entry_t;

typedef struct kernel_load_info
{
    uint32_t kernel_size;
    memory_map_entry_t* memory_map;
    uint8_t memory_map_length;
    uint32_t *page_directory;
    uint8_t *mm_bitmap;
} kernel_load_info_t;

#endif
