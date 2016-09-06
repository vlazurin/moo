#ifndef H_LIST
#define H_LIST

typedef struct list_node list_node_t;

typedef struct list_node
{
    list_node_t *next;
    list_node_t *prev;
} list_node_t;

void delete_from_list(void *list, void* node);

#define add_to_list(list, node) \
if (list != 0) \
{ \
    ((list_node_t*)node)->next = (list_node_t*)list; \
    ((list_node_t*)list)->prev = (list_node_t*)node; \
} \
list = node;

#endif
