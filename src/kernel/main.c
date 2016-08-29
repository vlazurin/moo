#include <stdbool.h>
#include "system.h"
#include "debug.h"
#include "string.h"
#include "interrupts.h"
#include "mm.h"
#include "pit.h"
#include "tasking.h"

extern kernel_load_info_t *kernel_params;
// set by linker
uint32_t *bss_start;
uint32_t *bss_end;

void main()
{
    init_debug_serial();

    uint32_t bss_size = &bss_end - &bss_start;
    if (bss_size > kernel_params->bss_size)
    {
        debug("Kernel BSS %h doesn't fit allocated space %h", bss_size, kernel_params->bss_size);
        hlt();
    }
    memset(&bss_start, 0, bss_size);

    init_interrupts();
    init_memory_manager(kernel_params);
    init_pit();
    init_tasking();

    debug("end of kernel main\n");
    while(true)
    {
    }
}
