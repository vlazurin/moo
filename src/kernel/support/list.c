#include <stddef.h>
#include <stdint.h>
#include "list.h"

void delete_from_list(void **list, void* node)
{
    list_node_t *next = ((list_node_t*)node)->next;
    list_node_t *prev = ((list_node_t*)node)->prev;

    if (*list == node)
    {
        *list = next;
        if (next != NULL)
        {
            next->prev = NULL;
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

void push_in_list(void **list, void* node)
{
    if (*list == NULL) {
        *list = node;
        return;
    }

    struct list_node *iterator = *list;
    while(iterator != NULL) {
        if (iterator->next == NULL) {
            iterator->next = node;
            ((struct list_node*)node)->prev = iterator;
            ((struct list_node*)node)->next = NULL;
            break;
        }
        iterator = (struct list_node*)iterator->next;
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
