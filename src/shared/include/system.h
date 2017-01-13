#ifndef H_SYSTEM_GENERIC
#define H_SYSTEM_GENERIC

#include <stdint.h>

#define ALIGN(n, a) (((n) + (a - 1)) & ~(a - 1))
#define PAGE_ALIGN(n) ALIGN(n, 0x1000)
#define MIN(a, b) ((a) < (b) ? (a) : (b))

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

typedef struct video_settings
{
    uint32_t *framebuffer;
    uint32_t width;
    uint32_t height;
} video_settings_t;

typedef struct page_directory
{
    // must be page aligned
    uint32_t directory[1024];
    uint32_t *pages[1024];
} page_directory_t;

#define KERNEL_SPACE_ADDR 0xC0000000
// 0x400000 - bytes in 4 mb (one page directory covers 4 mb of memory)
#define KERNEL_SPACE_START_PAGE_DIR (KERNEL_SPACE_ADDR / 0x400000)
#define KERNEL_VIRTUAL_ADDR 0xC0100000

#define PAGE_DIRECTORY_TOTAL_SIZE (PAGE_ALIGN(sizeof(page_directory_t)) + 0x400000)
#define PAGE_DIRECTORY_VIRTUAL (0x100000000 - PAGE_DIRECTORY_TOTAL_SIZE)

#define MM_BITMAP_SIZE 0x20000
// -0x1000 for unmapped page (overflow guard)
#define MM_BITMAP_VIRTUAL (PAGE_DIRECTORY_VIRTUAL - MM_BITMAP_SIZE - 0x1000)

#define KERNEL_STACK_SIZE 0x2000
// -0x1000 for unmapped page (overflow guard)
#define KERNEL_STACK (MM_BITMAP_VIRTUAL - KERNEL_STACK_SIZE - 0x1000)

// 100mb
#define KERNEL_HEAP_SIZE 0x6400000
#define KERNEL_HEAP (KERNEL_STACK - KERNEL_HEAP_SIZE - 0x1000)

// 100mb
#define HARDWARE_SPACE_SIZE 0x6400000
#define HARDWARE_SPACE (KERNEL_HEAP - HARDWARE_SPACE_SIZE - 0x1000)

#define KERNEL_BSS_SIZE 0x4000

typedef struct kernel_load_info
{
    memory_map_entry_t* memory_map;
    uint8_t memory_map_length;
    video_settings_t video_settings;
} kernel_load_info_t;

#endif
