#include <stddef.h>
#include "list.h"
#include "debug.h"

void delete_from_list(void *list, void* node)
{
    list_node_t *next =  ((list_node_t*)node)->next;
    list_node_t *prev =  ((list_node_t*)node)->prev;
    
    if (list == node)
    {
        list = next;
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
