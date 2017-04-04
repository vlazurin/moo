#ifndef H_LIST
#define H_LIST

typedef struct list_node list_node_t;

typedef struct list_node
{
    void *next;
    void *prev;
} list_node_t;

void delete_from_list(void **list, void* node);
uint32_t get_list_length(void *list);

#define add_to_list(list, node) \
if (list != 0) \
{ \
    ((list_node_t*)node)->next = (list_node_t*)list; \
    ((list_node_t*)node)->prev = 0; \
    ((list_node_t*)list)->prev = (list_node_t*)node; \
} \
list = node;

#define insert_in_list(node, after) \
((list_node_t*)node)->next = ((list_node_t*)after)->next; \
((list_node_t*)node)->prev = (list_node_t*)after; \
((list_node_t*)after)->next = (list_node_t*)node;

#define FOR_EACH(item, where, type) for (type *(item) = (where); (item); (item) = (type*)(item)->list.next)

#endif
