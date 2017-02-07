#include <stddef.h>
#include <stdint.h>
#include "list.h"
#include "log.h"

void delete_from_list(void **list, void* node)
{
    list_node_t *next = ((list_node_t*)node)->next;
    list_node_t *prev = ((list_node_t*)node)->prev;

    if (*list == node)
    {
        *list = next;
        if (next != NULL)
        {
            next->prev = 0;
        }
        return;
    }

    if (prev != NULL)
    {
        prev->next = next;
    }

    if (next != NULL)
    {
        next->prev = prev;
    }
}

uint32_t get_list_length(void *list)
{
    list_node_t *current = list;
    uint32_t length = 0;

    while(current != 0)
    {
        length++;
        current = current->next;
    }

    return length;
}
