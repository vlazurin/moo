#ifndef H_MM
#define H_MM

#include <stdint.h>
#include "system.h"

#define PAGE_PRESENT 1
#define PAGE_RW 2
#define PAGE_USER 4
#define PAGE_NO_CACHE 16

void init_memory_manager(kernel_load_info_t *kernel_params);
uint32_t alloc_physical_page();
uint32_t get_physical_address(uint32_t virtual);
uint32_t alloc_physical_range(uint16_t count);
void map_virtual_to_physical(uint32_t virtual, uint32_t physical, uint8_t flags);
void map_virtual_to_physical_range(uint32_t virtual, uint32_t physical, uint8_t flags, uint16_t count);
uint32_t alloc_hardware_space_chunk(int pages);
void mark_memory_region(uint32_t address, uint32_t size, uint8_t used);
void free_page(uint32_t virtual);
void free_physical_page(uint32_t phys);
void unmap_page(uint32_t virtual);
void free_userspace();

#endif
