#ifndef H_EVENT
#define H_EVENT

#define EVENT_TYPE_CUSTOM 12
#define MAX_EVENT_PACKET_SIZE 4096

struct event_data {
    struct list_node list;
    int sender;
    int target;
    uint16_t event_type;
    size_t size;
};

int init_event_loop();
int add_event_handler();
int remove_event_handler();

#endif
