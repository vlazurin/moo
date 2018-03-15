#ifndef H_MDM
#define H_MDM

#include <stdint.h>
#include <stddef.h>
#include "list.h"

#define MDM_REQUEST_CREATE_WINDOW 1
#define MDM_REQUEST_CLOSE_WINDOW 2
#define MDM_REQUEST_INVALIDATE_WINDOW 3

#define RESPONSE_OK 1

struct event_data {
    struct list_node list; // must be removed from userspace
    int sender;
    int target;
    int event_type;
    size_t size;
};

struct event_mdm {
    struct event_data event;
    int param1;
    int param2;
    int param3;
    int param4;
    int param5;
};

#endif
