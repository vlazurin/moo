#include <stddef.h>
#include "list.h"
#include "liballoc.h"
#include "vconsole.h"
#include "string.h"

void list_add(list_t *list, void *payload)
{
    mutex_lock(&list->mutex);
    list_node_t *node = kmalloc(sizeof(list_node_t));
    node->data = payload;
    node->next = list->root;
    if (list->root != 0)
    {
        list->root->prev = node;
    }
    list->root = node;
    mutex_release(&list->mutex);
}

void list_delete_node(list_t *list, list_node_t *node)
{
    if (node->prev != 0)
    {
        node->prev->next = node->next;
    }

    if (node->next != 0)
    {
        node->next->prev = node->prev;
    }

    if (list->root == node)
    {
        list->root = node->next;
        if (list->root != 0 && list->root->next != 0)
        {
            list->root->next->prev = 0;
        }
    }

    kfree(node);
}

void list_delete(list_t *list, void* data)
{
    mutex_lock(&list->mutex);
    list_node_t *node = list->root;
    while(node != 0)
    {
        if (node->data == data)
        {
            if (node->prev != 0)
            {
                node->prev->next = node->next;
            }

            if (node->next != 0)
            {
                node->next->prev = node->prev;
            }

            if (list->root == node)
            {
                list->root = node->next;
                if (list->root != 0 && list->root->next != 0)
                {
                    list->root->next->prev = 0;
                }
            }

            kfree(node);
            mutex_release(&list->mutex);
            return;
        }

        node = node->next;
    }

    mutex_release(&list->mutex);
}

void list_free(list_t *list)
{
    kfree(list);
}

list_t* create_list()
{
    list_t *list = kmalloc(sizeof(list_t));
    memset(list, 0, sizeof(list_t));

    list->add = &list_add;
    list->delete = &list_delete;
    list->delete_node = &list_delete_node;
    list->free = &list_free;

    return list;
}
