#ifndef H_MM
#define H_MM

#include <stdint.h>
#include "system.h"

void init_memory_manager(kernel_load_info_t *kernel_params);
uint32_t alloc_physical_page();
uint32_t get_physical_address(uint32_t virtual);
uint32_t alloc_physical_range(uint16_t count);
void map_virtual_to_physical(uint32_t virtual, uint32_t physical);
void map_virtual_to_physical_range(uint32_t virtual, uint32_t physical, uint16_t count);
uint32_t alloc_hardware_space_chunk(int pages);
void mark_memory_region(uint32_t address, uint32_t size, uint8_t used);
void free_page(uint32_t virtual);

#endif
