#ifndef H_EVENT
#define H_EVENT

#include "list.h"
#include <stddef.h>

#define MAX_EVENT_PACKET_SIZE 4096

struct event_data {
    struct list_node list;
    int sender;
    int target;
    uint16_t event_type;
    size_t size;
};

#endif
