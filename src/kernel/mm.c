#include "mm.h"
#include "debug.h"
#include "irq.h"
#include "mutex.h"
#include "string.h"
#include "liballoc.h"
#include "system.h"
#include "task.h"

uint8_t *bitmap = (uint8_t*) MM_BITMAP_VIRTUAL;
page_directory_t * volatile page_directory = (page_directory_t*)PAGE_DIRECTORY_VIRTUAL;
uint8_t *heap = (uint8_t*)KERNEL_HEAP;
uint8_t *hardware_space = (uint8_t*)HARDWARE_SPACE;
mutex_t liballoc_mutex = 0;
mutex_t mm_mutex = 0;

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
int liballoc_free(void* ptr, size_t pages)
{
    return 0;
}

void *liballoc_alloc(size_t pages)
{
    uint8_t *current = heap;
    if ((uint32_t)(heap + pages * 0x1000 - KERNEL_HEAP) >= KERNEL_HEAP_SIZE)
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

uint32_t alloc_hardware_space_chunk(int pages)
{
    uint32_t addr = (uint32_t)hardware_space;
    if ((uint32_t)(hardware_space + pages * 0x1000 - HARDWARE_SPACE) >= HARDWARE_SPACE_SIZE)
    {
        debug("kernel hardware space has no free space\n");
        hlt();
        //return 0;
    }

    hardware_space += pages * 0x1000;
    return addr;
}

static void page_fault_handler(struct regs *r)
{
    uint32_t virt;
    asm("movl %%cr2, %0" : "=r"(virt));

    debug("Page Fault at addr: %h, error code: %h, eip: %h\n", virt, r->error_code, r->eip);
    hlt();
    /*if (!(page_fault_handler_error_code & 1)) // page not set
    {
        uint32_t phys = alloc_physical_page();
        map_virtual_to_physical(virt, phys);
        debug("virtual %h mapped to %h\n", virt, phys);
    }
    else
    {
        debug("Page fault handler isn't finished...\n");
        hlt();
    }*/
}

void mark_memory_region(uint32_t address, uint32_t size, uint8_t used)
{
    assert(used == 0 || used == 1);
    uint32_t page = address / 0x1000;
    uint32_t count = PAGE_ALIGN(size) / 0x1000;
    //debug("[memory manager] mark region: address %h, start page %i, count %i, used flag %i\n", address, page, count, used);
    mutex_lock(&mm_mutex);
    for(uint32_t i = 0; i < count; i++)
    {
        if (used == 1)
        {
            bitmap[(page + i) / 8] |= (1 << ((page + i) % 8));
        }
        else
        {
            bitmap[(page + i) / 8] &= ~(1 << ((page + i) % 8));
        }
    }
    mutex_release(&mm_mutex);
}

void init_memory_manager(kernel_load_info_t *kernel_params)
{
    memset(bitmap, 0, MM_BITMAP_SIZE);

    for(uint8_t i = 0; i < kernel_params->memory_map_length; i++)
    {
        if (kernel_params->memory_map[i].type != MEMORY_MAP_REGION_FREE)
        {
            mark_memory_region(kernel_params->memory_map[i].base_low, kernel_params->memory_map[i].length_low, 1);
        }
    }

    set_irq_handler(0x0E, page_fault_handler);
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
                bitmap[i] |= (1 << x);
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
                    uint8_t ax = cx;
                    for(uint32_t ai = ci; count > 0; ai++)
                    {
                        for(; ax < 8 && count > 0; ax++)
                        {
                            bitmap[ai] |= (1 << ax);
                            count--;
                        }
                        ax = 0;
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

    if ((page_directory->directory[dir] & 1) == 0)
    {
        page_directory->page_chunks[dir] = kmalloc(0x1000 + 0x1000);
        page_directory->pages[dir] = (uint32_t*)PAGE_ALIGN((uint32_t)page_directory->page_chunks[dir]);
        for(uint32_t i = 0; i < 1024; i++)
        {
            page_directory->pages[dir][i] = 2;
        }

        page_directory->directory[dir] = get_physical_address((uint32_t)page_directory->pages[dir]) | 7;
    }

    page_directory->pages[dir][page] = physical | 7;
    asm volatile("invlpg (%0)" ::"r" (virtual) : "memory");

    // MUST IGNORE CALLS FOR KERNEL MEMORY, fix me
    if ((uint32_t)current_process->brk <= virtual && virtual < 0xE1C00000)
    {
        current_process->brk = (void*)virtual + 0x1000;
        debug("(PID: %i) new brk %h\n", get_pid(), current_process->brk);
    }
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
    // Should be mutex lock before IF? Probably value in page_table can be changed between IF and calc...
    if (page_directory->pages[dir] != 0 && (page_directory->pages[dir][page] & 7) == 7)
    {
        return (page_directory->pages[dir][page] & 0xFFFFF000) + (virtual & 0xFFF);
    }

    // TODO: not good idea to return 0, it can be correct value
    return 0;
}

void free_page(uint32_t virtual)
{
    uint32_t dir = (virtual >> 22);
    uint32_t page = (virtual >> 12) & 0x03FF;
    // Should be mutex lock before IF? Probably value in page_table can be changed between IF and calc...
    if (page_directory->pages[dir] != 0 && (page_directory->pages[dir][page] & 7) == 7) {
        uint32_t phys = (page_directory->pages[dir][page] & 0xFFFFF000) + (virtual & 0xFFF);
        uint32_t index = phys / 0x1000 / 8;
        bitmap[index] &= ~(1 << (phys / 0x1000 % 8));
        page_directory->pages[dir][page] = 2;
        asm volatile("invlpg (%0)" ::"r" (virtual) : "memory");
    }
}

void free_userspace()
{
    for(uint32_t i = 0; i < KERNEL_SPACE_START_PAGE_DIR; i++) {
        if (page_directory->directory[i] == 2) {
            continue;
        }
        for(uint32_t y = 0; y < 1024; y++) {
            if ((page_directory->pages[i][y] & 7) == 7) {
                free_page((i * 0x400 + y) * 0x1000);
            }
        }
    }
}
