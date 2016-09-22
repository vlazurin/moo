#include <stdbool.h>
#include "system.h"
#include "debug.h"
#include "string.h"
#include "interrupts.h"
#include "mm.h"
#include "pit.h"
#include "pci.h"
#include "tasking.h"
#include "timer.h"
#include "liballoc.h"
#include "tests.h"

extern kernel_load_info_t *kernel_params;
// set by linker
uint32_t *bss_start;
uint32_t *bss_end;

void main()
{
    init_debug_serial();

    uint32_t bss_size = &bss_end - &bss_start;
    if (bss_size > KERNEL_BSS_SIZE)
    {
        debug("Kernel BSS %h doesn't fit allocated space %h", bss_size, KERNEL_BSS_SIZE);
        hlt();
    }
    memset(&bss_start, 0, bss_size);

    init_interrupts();
    init_memory_manager(kernel_params);

    #ifdef DEBUG
    run_tests();
    #endif

    // copy kernel_params to high memory area
    uint32_t size = sizeof(kernel_load_info_t) + sizeof(memory_map_entry_t) * kernel_params->memory_map_length;
    void *ptr = kmalloc(size);
    memcpy(ptr, kernel_params, size);
    kernel_params = ptr;

    init_pit();
    init_tasking();
    init_timer();
    init_pci_devices();
    debug("[kernel] end of kernel main\n");

    while(true){}
}
