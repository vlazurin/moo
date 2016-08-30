#ifndef H_LIST
#define H_LIST

#include <stdint.h>
#include "mutex.h"

typedef struct list_node
{
    struct list_node *next;
    struct list_node *prev;
    void *data;
} list_node_t;

typedef struct list
{
    list_node_t *root;
    mutex_t mutex;
    void (*add)(struct list*, void*);
    void (*delete)(struct list*, void*);
    void (*delete_node)(struct list*, list_node_t*);
    void (*free)(struct list*);
} list_t;

list_t* create_list();

#endif
