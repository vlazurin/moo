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
#include "network.h"
#include "dhcp.h"
#include "tcp.h"

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

    pci_device_t *dev = get_pci_device_by_class(PCI_CLASS_NETWORK_CONTROLLER);
    network_device_t *net_dev = (network_device_t*)dev->logical_driver;
    if (net_dev != 0)
    {
        init_udp_protocol();
        init_tcp_protocol();
        net_dev->enable(net_dev);
        uint8_t result = configure_dhcp(net_dev);
        if (result != DHCP_OK)
        {
            debug("[network] DHCP configuration failed, status: %i\n", result);
        }
        else
        {
            char buf[16];
            ip4_to_str(&net_dev->ip4_addr, buf);
            debug("[network] network configuration finished\n");
            debug("[network] my ip: %s\n", buf);
            ip4_to_str(&net_dev->subnet_mask, buf);
            debug("[network] network mask: %s\n", buf);
            ip4_to_str(&net_dev->router, buf);
            debug("[network] default router: %s\n", buf);
            debug("[network] DHCP leasing time isn't supported!\n");
        }
    }

    debug("[kernel] end of kernel main\n");

    while(true){}
}
