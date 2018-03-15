#ifndef H_GUI
#define H_GUI

#include <stdint.h>
#include <stddef.h>
#include "primitives.h"
#include "list.h"

#define WINDOW_FLAG_FULL_SCREEN (1 << 0)
#define WINDOW_FLAG_ALWAYS_VISIBLE (1 << 1)

struct control {
    struct list_node list;
    uint32_t id;
    void (*draw)(struct context* ctx);
    struct rect location;
    uint32_t background_color;
    uint32_t border_color;
};

struct window {
    uint32_t id;
    void (*draw)();
    struct context* ctx;
    int x;
    int y;
    int width;
    int height;
    struct control *children;
};

struct label {
    struct control control;
};

int init_gui();
int create_window(size_t width, size_t height, uint32_t flags);
int close_window(uint32_t window_id);
int redraw_window(uint32_t window_id);
void* get_draw_context(uint32_t window_id);

#endif
