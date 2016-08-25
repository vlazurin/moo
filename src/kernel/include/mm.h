#ifndef H_MM
#define H_MM

#include <stdint.h>
#include "system.h"

#define MM_BITMAP_SIZE 0x20000

void init_memory_manager(kernel_load_info_t *kernel_params);
uint32_t alloc_physical_page();
uint32_t get_physical_address(uint32_t virtual);
uint32_t alloc_physical_range(uint16_t count);
void map_virtual_to_physical(uint32_t virtual, uint32_t physical);
void map_virtual_to_physical_range(uint32_t virtual, uint32_t physical, uint16_t count);

#endif
