#include <stdint.h>
#include "bios.h"
#include "fat16.h"
#include "system.h"
#include "string.h"
#include "debug.h"

/*
Lower memory map:
0x00000000 - 0x000003FF (1kb) Interrupt vector table
0x00000400 - 0x000004FF (256 bytes) BIOS data area
0x00000500 - 0x0007FFFF (>480kb) free memory
...............

Code & data layout:
0x0500 - 0x7c00 stack
0x7c00 - 0x7e00 bootsector
0x7e00 - 0x???? second stage loader
0x10000 - 0x18000 root directory
0x18000 - 0x38000 FAT 16
0x38000 - 0x38960 memory map (with soft limit 100 entries) + some reserved memory for custom added entries
0x40000 - 0x????? buffer for lba read
*/

#define PAGE_DIRECTORY_TOTAL_SIZE 0x1000000
#define PAGE_DIRECTORY_VIRTUAL 0xFA000000
#define MM_BITMAP_VIRTUAL 0xFEFDF000
#define MM_BITMAP_SIZE 0x20000
#define LOAD_BUFFER 0x40000
#define KERNEL_BSS_SIZE 0x4000
#define MEMORY_MAP_MAX_ADDRESS 0x40000

memory_map_entry_t* memory_map = (memory_map_entry_t*)0x38000;
uint8_t memory_map_length = 0;
kernel_load_info_t kernel_params;

uint32_t *page_directory = 0;

// linker will set values(see os_loader.ld)
uint32_t *bss_start;
uint32_t *bss_end;

void fill_paging_info(uint32_t virtual, uint32_t physical, uint32_t pages_count)
{
    virtual = virtual & 0xFFFFF000;

    for(uint32_t i = 0; i < pages_count; i++)
    {
        uint32_t dir = (virtual >> 22);
        uint32_t page = (virtual >> 12) & 0x03FF;
        uint32_t *pages = (uint32_t*)(page_directory[dir] & ~3);
        pages[page] = physical | 3;
        virtual += 0x1000;
        physical += 0x1000;
    }
}

void add_to_memory_map(uint32_t base, uint32_t length, uint8_t type)
{
    memory_map_entry_t *last = memory_map + memory_map_length;

    if ((uintptr_t)last >= MEMORY_MAP_MAX_ADDRESS)
    {
        print("Not enough free space for memory map");
        hlt();
    }

    last->base_low = base;
    last->length_low = length;
    last->type = type;
    memory_map_length++;
}

void main()
{
    // zero all global vars with 0 value
    memset(&bss_end, 0, &bss_end - &bss_start);

    memory_map_length = detect_upper_memory(memory_map);
    // first 1mb of memory
    add_to_memory_map(0x0, 0x100000, MEMORY_MAP_REGION_RESERVED);

    init_fat();

    fat_entry_t* kernel = get_fat_entry("KERNEL", "");
    if (kernel == 0)
    {
        print("Kernel isn't found");
        hlt();
    }

    uint16_t cluster_size = bytes_per_cluster();
    void *load_addr = 0;
    uint8_t *kernel_entry_point = 0;
    uint32_t kernel_size = PAGE_ALIGN(kernel->file_size + KERNEL_BSS_SIZE);
    uint32_t required_memory = kernel_size + PAGE_DIRECTORY_TOTAL_SIZE + MM_BITMAP_SIZE;

    for(uint8_t i = 0; i < memory_map_length; i++)
    {
        // ignore lower memory
        if (memory_map[i].base_low < 0x100000)
        {
            continue;
        }

        uint32_t free_memory = memory_map[i].length_low - PAGE_ALIGN(memory_map[i].base_low);

        if (memory_map[i].type == MEMORY_MAP_REGION_FREE && required_memory <= free_memory)
        {
            load_addr = (void*)PAGE_ALIGN(memory_map[i].base_low);
            kernel_entry_point = load_addr;
            void *buffer = (void*)LOAD_BUFFER;
            uint16_t cluster = kernel->start_cluster;
            while (cluster != 0)
            {
                cluster = read_cluster(cluster, buffer);
                memcpy(load_addr, buffer, cluster_size);
                load_addr += cluster_size;
            }
            break;
        }
    }

    if (load_addr == 0) {
        print("Not enough free memory");
        hlt();
    }

    load_addr = (void*)PAGE_ALIGN((uintptr_t)load_addr + KERNEL_BSS_SIZE);
    page_directory = (uint32_t*)load_addr;
    uint32_t *page_table = 0;
    for(uint16_t i = 0; i < 1024; i++)
    {
        page_table = page_directory + 0x400 + 0x400 * i;
        page_directory[i] = (uint32_t)page_table | 3;
        for(uint16_t y = 0; y < 1024; y++)
        {
            page_table[y] = 0 | 2;
        }
    };

    // map the first 1MB of memory
    fill_paging_info(0, 0, 256);
    fill_paging_info(0xE1C00000, (uint32_t)kernel_entry_point, kernel_size / 0x1000);
    fill_paging_info(PAGE_DIRECTORY_VIRTUAL, (uint32_t)page_directory, PAGE_ALIGN(PAGE_DIRECTORY_TOTAL_SIZE) / 0x1000);
    fill_paging_info(MM_BITMAP_VIRTUAL, (uint32_t)page_directory + PAGE_DIRECTORY_TOTAL_SIZE, PAGE_ALIGN(MM_BITMAP_SIZE) / 0x1000);

    add_to_memory_map((uint32_t)kernel_entry_point, required_memory, MEMORY_MAP_REGION_RESERVED);

    gdt_init();
    gdt_set_gate(0, 0, 0, 0, 0);
    // code segment, 4 kb, 32 bit opcode, supervisor
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);
    // data segment, supervisor
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);
    set_gdt();

    kernel_params.kernel_size = kernel->file_size;
    kernel_params.page_directory = (uint32_t*)PAGE_DIRECTORY_VIRTUAL;
    kernel_params.mm_bitmap = (uint8_t*)MM_BITMAP_VIRTUAL;
    kernel_params.memory_map = memory_map;
    kernel_params.memory_map_length = memory_map_length;
    kernel_params.bss_size = KERNEL_BSS_SIZE;
    set_video_mode(&kernel_params.video_settings);
    
    // enable protected mode
    asm("cli");
    asm("movl %cr0, %eax");
    asm("orl $0x1, %eax");
    asm("movl %eax, %cr0");
    asm("ljmp $0x08, $protected_mode_enty");

    asm(".code32");
    asm("protected_mode_enty:");

    asm("movw	$0x10,  %ax");
    asm("movw	%ax,	%ds");
    asm("movw	%ax,	%es");
    asm("movw	%ax,	%fs");
    asm("movw	%ax,	%gs");
    asm("movw	%ax,	%ss");
    asm("movw	$0x4000,  %ax");
    asm("movw	%ax,	%sp");

    asm("movl page_directory, %eax");
    asm("mov %eax, %cr3");
    asm("mov %cr0, %eax");
    asm("or $0x80000000, %eax");
    asm("mov %eax, %cr0");

    asm("movl %0, %%ebx" :: "r"(&kernel_params) : "%ebx");
    asm("ljmp $0x08, $0xE1C00000");
}
