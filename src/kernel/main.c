#include <stdbool.h>
#include "system.h"
#include "debug.h"
#include "string.h"

extern kernel_load_info_t *kernel_params;
// set by linker
uint32_t bss_start;
uint32_t bss_end;

void main()
{
    uint32_t bss_size = bss_end - bss_start;
    if (bss_size > kernel_params->bss_size)
    {
        debug("Kernel BSS doesn't fit allocated space");
        hlt();
    }

    memset((void*)bss_start, 0, bss_size);

    while(true)
    {
    }
}
