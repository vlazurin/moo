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

#define LOAD_BUFFER 0x40000
#define MEMORY_MAP_MAX_ADDRESS 0x40000

memory_map_entry_t* memory_map = (memory_map_entry_t*)0x38000;
uint8_t memory_map_length = 0;
kernel_load_info_t kernel_params;

page_directory_t *page_directory = 0;

#define GDT_SIZE 3

struct gdt_entry gdt_entries[GDT_SIZE];
struct gdt_register gdt_reg;

// linker will set values(see os_loader.ld)
// variable size must be 1 byte, or (&bss_end - &bss_start) will give wrong result
extern char bss_start;
extern char bss_end;

void fill_paging_info(uint32_t virtual, uint32_t physical, uint32_t pages_count)
{
    virtual = virtual & 0xFFFFF000;
    //debug("[loader] mapping %h - %h to physical %h - %h\n",
    //    virtual, virtual + pages_count * 0x1000 - 1, physical, physical + pages_count * 0x1000 - 1);
    for(uint32_t i = 0; i < pages_count; i++)
    {
        uint32_t dir = (virtual >> 22);
        uint32_t page = (virtual >> 12) & 0x03FF;
        uint32_t *pages = (uint32_t*)(page_directory->directory[dir] & ~7);
        pages[page] = physical | 7;
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
    //debug("[loader] bss size is %h\n", &bss_end - &bss_start);
    memory_map_length = detect_upper_memory(memory_map);
    // first 1mb of memory
    add_to_memory_map(0x0, 0x100000, MEMORY_MAP_REGION_RESERVED);

    init_fat();

    fat_entry_t* kernel = get_fat_entry("KERNEL", "");
    if (kernel == 0)
    {
        debug("[loader] kernel isn't found\n");
        print("Kernel isn't found");
        hlt();
    }

    uint16_t cluster_size = bytes_per_cluster();
    void *load_addr = 0;
    uint8_t *kernel_entry_point = 0;
    uint32_t kernel_size = PAGE_ALIGN(kernel->file_size + KERNEL_BSS_SIZE + 0x1000); // Page Fault without +0x1000
    //debug("[loader] kernel size is %h + %h bss\n", kernel->file_size, KERNEL_BSS_SIZE);
    uint32_t required_memory = kernel_size + PAGE_DIRECTORY_TOTAL_SIZE + MM_BITMAP_SIZE + KERNEL_STACK_SIZE;
    //debug("[loader] total required memory: %h\n", required_memory);
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
            //debug("[loader] loading data at %h\n", load_addr);
            kernel_entry_point = load_addr;
            void *buffer = (void*)LOAD_BUFFER;
            uint32_t loaded = 0;
            uint16_t cluster = kernel->start_cluster;
            while (cluster != 0)
            {
                cluster = read_cluster(cluster, buffer);
                memcpy(load_addr, buffer, cluster_size);
                load_addr += cluster_size;
                loaded += cluster_size;
            }
            //debug("[loader] loaded %i, kernel size is %i\n", loaded, kernel->file_size);
            break;
        }
    }

    if (load_addr == 0) {
        debug("[loader] not enough free memory\n");
        print("Not enough free memory");
        hlt();
    }

    load_addr = (void*)PAGE_ALIGN((uintptr_t)load_addr + KERNEL_BSS_SIZE);
    page_directory = (page_directory_t*)(load_addr + 0xF000); // + 0x1000 because current page is a last BSS page
    debug("[loader] page directory location is %h\n", load_addr);
    uint32_t *page_table = 0;

    for(uint16_t i = 0; i < 1024; i++)
    {
        page_table = (void*)page_directory + PAGE_ALIGN(sizeof(page_directory_t)) + 0x1000 * i;
        page_directory->directory[i] = (uint32_t)page_table | 7;
        for(uint16_t y = 0; y < 1024; y++)
        {
            page_table[y] = 0 | 2;
        }
        page_directory->pages[i] = (void*)PAGE_DIRECTORY_VIRTUAL + PAGE_ALIGN(sizeof(page_directory_t)) + 0x1000 * i;
    };

    // map the first 1MB of memory
    fill_paging_info(0, 0, 256);
    fill_paging_info(KERNEL_VIRTUAL_ADDR, (uint32_t)kernel_entry_point, kernel_size / 0x1000);
    fill_paging_info(PAGE_DIRECTORY_VIRTUAL, (uint32_t)page_directory, PAGE_DIRECTORY_TOTAL_SIZE / 0x1000);
    fill_paging_info(MM_BITMAP_VIRTUAL, (uint32_t)page_directory + PAGE_DIRECTORY_TOTAL_SIZE + 0x1000, MM_BITMAP_SIZE / 0x1000);
    fill_paging_info(KERNEL_STACK, (uint32_t)page_directory + PAGE_DIRECTORY_TOTAL_SIZE + MM_BITMAP_SIZE  + 0x2000, KERNEL_STACK_SIZE / 0x1000);

    add_to_memory_map((uint32_t)kernel_entry_point, required_memory, MEMORY_MAP_REGION_RESERVED);

    gdt_set_gate(gdt_entries, 0, 0, 0, 0, 0);
    // code segment, 4 kb, 32 bit opcode, supervisor
    gdt_set_gate(gdt_entries, 1, 0, 0xFFFFFFFF, 0x9A, 0xCF);
    // data segment, supervisor
    gdt_set_gate(gdt_entries, 2, 0, 0xFFFFFFFF, 0x92, 0xCF);

    gdt_reg.limit = (sizeof(struct gdt_entry) * GDT_SIZE) - 1;
    gdt_reg.base = (uint32_t)gdt_entries;
    asm("lgdt gdt_reg");

    kernel_params.memory_map = memory_map;
    kernel_params.memory_map_length = memory_map_length;
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
    asm("movl %0, %%eax" :: "r"(KERNEL_STACK + KERNEL_STACK_SIZE) : "%eax");
    asm("mov %eax, %esp");
    asm("movl %0, %%ebx" :: "r"(&kernel_params) : "%ebx");
    asm("ljmp $0x08, %0" :: "i"(KERNEL_VIRTUAL_ADDR));
}
