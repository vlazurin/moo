#include <stdbool.h>
#include "irq.h"
#include "system.h"
#include "log.h"
#include "string.h"
#include "screen.h"
#include "mm.h"
#include "pit.h"
#include "pci.h"
#include "task.h"
#include "timer.h"
#include "liballoc.h"
#include "tests.h"
#include "network.h"
#include "dhcp.h"
#include "tcp.h"
#include "vfs.h"
#include "tempfs.h"
#include "fat16fs.h"
#include "procfs.h"
#include "log.h"
#include "elf.h"
#include "tty.h"
#include "kb.h"
#include "shm.h"
#include "tss.h"
#include "gdt.h"

void setup_syscalls();
extern kernel_load_info_t *kernel_params;
// set by linker
uint32_t *bss_start;
uint32_t *bss_end;

void init_serial();
void init_urandom();
void init_null();
void init_io();

uint8_t exec(char *path);

void main()
{
    init_early_log();

    uint32_t bss_size = &bss_end - &bss_start;
    if (bss_size > KERNEL_BSS_SIZE) {
        log(KERN_FATAL, "kernel BSS (%x) doesn't fit allocated space (%x)", bss_size, KERNEL_BSS_SIZE);
        hlt();
    }

    memset(&bss_start, 0, bss_size);
    asm("finit"); // because bochs is very annoying about "MSDOS compatibility FPU exception"
    init_irq();
    init_memory_manager(kernel_params);

    init_gdt();

    #ifdef RUN_TESTS
    run_tests();
    #endif

    // copy kernel_params to high memory area
    uint32_t size = sizeof(kernel_load_info_t) + sizeof(memory_map_entry_t) * kernel_params->memory_map_length;
    void *ptr = kmalloc(size);
    memcpy(ptr, kernel_params, size);
    kernel_params = ptr;

    init_pit();
    init_multitasking();
    init_timer();

    init_tempfs();
    uint8_t error = mount_fs("/", "tempfs", NULL);
    if (error) {
        log(KERN_FATAL, "can't mount root partition (errno: %i)\n", error);
        hlt();
    }
    mkdir("/dev");
    mkdir("/mount");

    init_serial();
    init_urandom();
    init_null();

    init_pci_devices();
    init_fat16fs();
    init_screen();
    init_keyboard();
    init_shm();
    /*pci_device_t *dev = get_pci_device_by_class(PCI_CLASS_NETWORK_CONTROLLER);
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
    }*/

    setup_syscalls();
    init_io();
    symlink("/bin", "/mount/NO NAME/bin");
    symlink("/home", "/mount/NO NAME/home");
    symlink("/etc", "/mount/NO NAME/etc");
    exec("/bin/dash");
    while(true){}
}
