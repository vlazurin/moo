#ifndef H_MDM
#define H_MDM

#include <stdint.h>
#include "primitives.h"

#define REQUEST_CREATE_WINDOW 1
#define REQUEST_CLOSE_WINDOW 2
#define REQUEST_INVALIDATE_WINDOW 3

#define RESPONSE_OK 1

#define WINDOW_HEADER_HEIGHT 24
#define WINDOW_BORDER_WIDTH 1

// TODO: move me to system headers
struct event_data {
    struct list_node list;
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

struct window {
    struct list_node list;
    struct rect w_rect; // window area
    struct rect c_rect; // client area
    uint32_t pid;
    uint32_t id;
    void *buffers[2];
    uint8_t active_buffer;
    uint32_t flags;
};

struct mdm_state {
    uint32_t surface_width;
    uint32_t surface_height;
    uint8_t bbp;
    struct window *windows;
    uint32_t next_id;
    uint8_t render_lock;
    cairo_surface_t *cairo_surface;
    cairo_t *cairo;
    void *fb;
    int events_file;
    struct {
        cairo_surface_t *close_button;
    } sprites;
};

#endif
