#include <stdint.h>
#include "list.h"
#include "debug.h"
#include "liballoc.h"
#include "string.h"
#include "system.h"
#include "mm.h"

typedef struct test_node
{
    list_node_t list;
    uint32_t value;
} test_node_t;

void test_list()
{
    test_node_t *list = 0;

    test_node_t *first = kmalloc(sizeof(test_node_t));
    first->value = 1;
    test_node_t *second = kmalloc(sizeof(test_node_t));
    second->value = 2;
    test_node_t *third = kmalloc(sizeof(test_node_t));
    third->value = 3;

    add_to_list(list, first);
    assert(list == first);
    assert(list->list.next == 0);
    assert(list->list.prev == 0);
    assert(list->value == 1);

    add_to_list(list, second);
    assert(list == second);
    assert(list->list.next == (list_node_t*)first);
    assert(list->list.prev == 0);
    assert(list->value == 2);

    add_to_list(list, third);
    assert(list->list.next == (list_node_t*)second);
    assert(second->list.next == (list_node_t*)first);
    assert(list->value == 3);

    // remove item from middle (third -> second -> first)
    delete_from_list((void*)&list, second);
    assert(list->list.next == (list_node_t*)first);
    assert(first->list.prev == (list_node_t*)list);
    assert(list->value == 3);

    // remove first item (third -> first)
    delete_from_list((void*)&list, third);

    assert(list == first);
    assert(list->list.next == 0);
    assert(list->list.prev == 0);

    add_to_list(list, second);
    // remove last item (second -> first)
    delete_from_list((void*)&list, first);
    assert(list == second);
    assert(list->list.next == 0);

    delete_from_list((void*)&list, second);
    assert(list == 0);
}

extern uint8_t *bitmap;
// risky test, because memory bitmap is replaced by dummy, so any real usage can cause system crash
void test_mm_mark_memory_region()
{
    asm("cli");
    uint8_t *old = bitmap;
    uint8_t *new_bitmap = kmalloc(MM_BITMAP_SIZE);
    memset(new_bitmap, 0, MM_BITMAP_SIZE);
    bitmap = new_bitmap;

    mark_memory_region(0, 0, 1);
    assert(bitmap[0] == 0 && bitmap[1] == 0);
    mark_memory_region(0, 1, 1);
    assert(bitmap[0] == 1 && bitmap[1] == 0);
    mark_memory_region(0xFFFFFFFF - 0x2000, 0x2000, 1);
    assert(bitmap[MM_BITMAP_SIZE - 1] == 0x60);
    mark_memory_region(0xFFFFFFFF - 0x2000, 0x1c00, 0);
    assert(bitmap[MM_BITMAP_SIZE - 1] == 0x0);

    kfree(new_bitmap);
    bitmap = old;
    asm("sti");
}

// risky test, because memory bitmap is replaced by dummy, so any real usage can cause system crash
void test_alloc_physical_range()
{
    asm("cli");
    uint8_t *old = bitmap;
    uint8_t *new_bitmap = kmalloc(MM_BITMAP_SIZE);
    memset(new_bitmap, 0, MM_BITMAP_SIZE);
    bitmap = new_bitmap;

    mark_memory_region(0, 0x9000, 1);
    uint32_t __attribute__((unused)) addr = alloc_physical_range(3);
    assert(addr == 0x9000);
    addr = alloc_physical_range(1);
    assert(addr == 0xC000);
    mark_memory_region(0xE000, 0x1000, 1);
    addr = alloc_physical_range(2);
    assert(addr == 0xF000);
    kfree(new_bitmap);
    bitmap = old;
    asm("sti");
}

#include "arp.h"
extern arp_entry_t *arp_cache;

void test_get_mac_from_cache()
{
    eth_addr_t addr = ETH_ADDR_EMPTY;
    ip4_addr_t ip = BUILD_IP4_ADDR(192, 168, 0, 2);

    arp_cache = 0;

    uint8_t __attribute__((unused)) result = get_mac_from_cache(&ip, &addr);
    assert(result == 0);

    arp_entry_t entry;
    entry.ip_addr = ip;
    eth_addr_t tmp = {{19, 200, 20, 34, 23, 17}};
    entry.mac = tmp;
    add_to_list(arp_cache, &entry);
    arp_entry_t entry2;
    ip4_addr_t ip2 = BUILD_IP4_ADDR(192, 168, 0, 3);
    entry2.ip_addr = ip2;
    eth_addr_t tmp2 = {{34, 89, 01, 34, 23, 70}};
    entry2.mac = tmp2;
    add_to_list(arp_cache, &entry2);

    result = get_mac_from_cache(&ip, &addr);
    assert(result == 1);
    assert(addr.b[0] == 19);
    assert(addr.b[5] == 17);

    result = get_mac_from_cache(&ip2, &addr);
    assert(result == 1);
    assert(addr.b[0] == 34);
    assert(addr.b[5] == 70);

    arp_cache = 0;
}

void test_add_mac_to_arp_cache()
{
    eth_addr_t mac = {{19, 200, 20, 34, 23, 17}};
    ip4_addr_t ip = BUILD_IP4_ADDR(192, 168, 0, 1);
    eth_addr_t mac2 = {{20, 200, 20, 34, 23, 117}};
    ip4_addr_t ip2 = BUILD_IP4_ADDR(192, 168, 0, 2);

    arp_cache = 0;

    add_mac_to_arp_cache(&ip, &mac);
    eth_addr_t __attribute__((unused)) out = ETH_ADDR_EMPTY;
    assert(get_mac_from_cache(&ip, &out));
    assert(out.b[0] == 19);
    assert(out.b[5] == 17);

    add_mac_to_arp_cache(&ip2, &mac2);
    assert(get_mac_from_cache(&ip2, &out));
    assert(out.b[0] == 20);
    assert(out.b[5] == 117);

    mac2.b[0] = 21;
    add_mac_to_arp_cache(&ip2, &mac2);
    assert(get_mac_from_cache(&ip2, &out));
    assert(out.b[0] == 21);
    assert(out.b[5] == 117);
    assert(get_list_length(arp_cache) == 2);
    arp_cache = 0;
}

void run_tests()
{
    test_list();
    test_mm_mark_memory_region();
    test_alloc_physical_range();
    test_get_mac_from_cache();
    test_add_mac_to_arp_cache();
}
