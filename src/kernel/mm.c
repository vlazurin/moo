#include "mm.h"
#include "debug.h"
#include "interrupts.h"
#include "mutex.h"
#include "string.h"
#include "system.h"

#define HEAD_ADDRESS 0xF8C00000

uint8_t *bitmap;
uint32_t *page_directory;
uint8_t *heap = (uint8_t*)HEAD_ADDRESS;
mutex_t liballoc_mutex = 0;
mutex_t mm_mutex = 0;

// heap 100mb
#define HEAP_SIZE 104857600

int liballoc_lock()
{
	mutex_lock(&liballoc_mutex);
	return 0;
}

int liballoc_unlock()
{
	mutex_release(&liballoc_mutex);
	return 0;
}

// TODO: should respect memory map... reject attempts to free reserved areas
int liballoc_free(void* ptr, int pages)
{
    return 0;
}

void *liballoc_alloc(int pages)
{
    uint8_t *current = heap;
    if ((uint32_t)(heap + pages * 0x1000 - HEAD_ADDRESS) >= HEAP_SIZE)
	{
		debug("kernel heap has no free space\n");
		hlt();
        //return 0;
    }

    heap += pages * 0x1000;

	for(uint32_t i = 0; i < pages; i++)
	{
		uint32_t addr = alloc_physical_page();
		map_virtual_to_physical((uint32_t)current + i * 0x1000, addr);
	}

	return current;
}

extern uint32_t page_fault_handler_error_code; // tmp solution, IRQ_HANDLER macro sets value for us
IRQ_HANDLER(page_fault_handler, 1)
{
    uint32_t virt;
    asm("movl %%cr2, %0" : "=r"(virt));

    debug("Page Fault at addr: %h, error code: %h\n", virt, page_fault_handler_error_code);
    if (!(page_fault_handler_error_code & 1)) // page not set
    {
        uint32_t phys = alloc_physical_page();
        map_virtual_to_physical(virt, phys);
        debug("virtual %h mapped to %h\n", virt, phys);
    }
    else
    {
        debug("Page fault handler isn't finished...\n");
        hlt();
    }
}

void mark_memory_region(uint32_t address, uint32_t size, uint8_t used)
{
    uint32_t start_page = address / 0x1000;
    uint32_t end_page = (address + size) / 0x1000;
	mutex_lock(&mm_mutex);
    for(uint32_t page = start_page; page < end_page; page++)
    {
        bitmap[page / 8] |= 1 << (page % 8);
    }
	mutex_release(&mm_mutex);
}

void init_memory_manager(kernel_load_info_t *kernel_params)
{
    bitmap = kernel_params->mm_bitmap;
    page_directory = kernel_params->page_directory;
    memset(bitmap, 0, MM_BITMAP_SIZE);

	for(uint8_t i = 0; i < kernel_params->memory_map_length; i++)
	{
		if (kernel_params->memory_map[i].type != MEMORY_MAP_REGION_FREE)
		{
			mark_memory_region(kernel_params->memory_map[i].base_low, kernel_params->memory_map[i].length_low, 1);
		}
	}

    disable_interrupts();
    set_interrupt_gate(0x0E, page_fault_handler, 0x08, 0x8E);
    enable_interrupts();
}

uint32_t alloc_physical_page()
{
	mutex_lock(&mm_mutex);
    for(uint32_t i = 0; i < MM_BITMAP_SIZE; i++)
    {
        if (bitmap[i] == 0xFF)
        {
            continue;
        }

        for(uint8_t x = 0; x < 8; x++)
        {
            if (!(bitmap[i] & (1 << x)))
            {
                bitmap[i] |= 1 << x;
				mutex_release(&mm_mutex);
                return ((i * 8 + x) * 0x1000);
            }
        }
    }

    debug("No free physical memory");
	// Actually swapping must be done here...
    hlt();
	mutex_release(&mm_mutex);
	return 0;
}

uint32_t alloc_physical_range(uint16_t count)
{
	if (count == 0)
	{
		return 0;
	}

	uint16_t found = 0;
	uint32_t ci = 0;
	uint32_t cx = 0;
	mutex_lock(&mm_mutex);
    for(uint32_t i = 0; i < MM_BITMAP_SIZE; i++)
    {
        if (bitmap[i] == 0xFF)
        {
            continue;
        }

        for(uint8_t x = 0; x < 8; x++)
        {
            if (!(bitmap[i] & (1 << x)))
            {
                if (found == 0)
				{
					ci = i;
					cx = x;
				}
				found++;
				if (found == count)
				{
					for(uint32_t ai = ci; ai < count; ai++)
				    {
				        for(uint8_t ax = cx; ax < 8; ax++)
				        {
							bitmap[ai] |= 1 << ax;
						}
					}
					mutex_release(&mm_mutex);
					return ((ci * 8 + cx) * 0x1000);
				}
            }
			else
			{
				found = 0;
			}
        }
    }

	mutex_release(&mm_mutex);
	if (found < count)
	{
	    debug("No free physical memory");
	    hlt();
	}
	return 0;
}


void map_virtual_to_physical(uint32_t virtual, uint32_t physical)
{
    uint32_t dir = (virtual >> 22);
    uint32_t page = (virtual >> 12) & 0x03FF;
    uint32_t *page_table = page_directory + 0x400 * dir + 0x400;
    page_table[page] = physical | 3;
    asm volatile("invlpg (%0)" ::"r" (virtual) : "memory");
}

void map_virtual_to_physical_range(uint32_t virtual, uint32_t physical, uint16_t count)
{
    for(uint16_t i = 0; i < count; i++)
	{
		map_virtual_to_physical(virtual + i * 0x1000, physical + i * 0x1000);
	}
}

uint32_t get_physical_address(uint32_t virtual)
{
	uint32_t dir = (virtual >> 22);
    uint32_t page = (virtual >> 12) & 0x03FF;
    uint32_t *page_table = page_directory + 0x400 * dir + 0x400;
	// Should be mutex lock before IF? Probably value in page_table can be changed between IF and calc...
	if ((page_table[page] & 3) == 3)
	{
		return (page_table[page] & 0xFFFFF000) + (virtual & 0xFFF);
	}

	return 0;
}
