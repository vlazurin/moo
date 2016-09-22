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

void run_tests()
{
    test_list();
    test_mm_mark_memory_region();
}
