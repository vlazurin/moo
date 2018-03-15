#include <stdbool.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <cairo/cairo.h>
#include "list.h"
#include "mdm.h"
#include "drawing.h"

// move me to gui.h
#define WINDOW_FLAG_FULL_SCREEN (1 << 0)
#define WINDOW_FLAG_ALWAYS_VISIBLE (1 << 1)

int shm_map(char *);
uintptr_t* shm_get_addr(char *);
extern int errno;

struct mdm_state global_state;

void lock(uint8_t *lock)
{
    while (__sync_lock_test_and_set(lock, 1)){}
}

void unlock(uint8_t *lock)
{
     __sync_lock_release(lock);
}

void redraw()
{
    lock(&global_state.render_lock);

    cairo_set_source_rgba(global_state.cairo, 0, 0, 0, 1);
    cairo_rectangle(global_state.cairo, 0, 0, global_state.surface_width, global_state.surface_height);
    cairo_fill(global_state.cairo);

    cairo_set_line_width(global_state.cairo, 1);
    struct window *w = 0;
    FOR_EACH(tmp, global_state.windows, struct window) {
        w = tmp;
    }
    while(w != NULL) {
    /*    if (!(w->flags & WINDOW_FLAG_FULL_SCREEN)) {
            if (w == global_state.windows) {
                cairo_set_source_rgba(global_state.cairo, 0.43, 0.77, 0.87, 1);
            } else {
                cairo_set_source_rgba(global_state.cairo, 0.7, 0.7, 0.7, 1);
            }
            cairo_rectangle(global_state.cairo, w->w_rect.x, w->w_rect.y, w->w_rect.width, WINDOW_HEADER_HEIGHT);
            cairo_fill(global_state.cairo);
            cairo_rectangle(global_state.cairo, w->w_rect.x, w->w_rect.y, w->w_rect.width, w->w_rect.height);
            cairo_stroke(global_state.cairo);
            cairo_set_source_surface(global_state.cairo, global_state.sprites.close_button, w->w_rect.x + w->w_rect.width - 16 - 5, w->w_rect.y + 4);
            cairo_paint(global_state.cairo);
        }*/

        int stride = cairo_format_stride_for_width (CAIRO_FORMAT_ARGB32, w->c_rect.width);
        cairo_surface_t *s = cairo_image_surface_create_for_data(w->buffers[w->active_buffer], CAIRO_FORMAT_ARGB32, w->c_rect.width, w->c_rect.height, stride);
        cairo_save(global_state.cairo);
        cairo_set_source_surface (global_state.cairo, s, w->c_rect.x, w->c_rect.y);
        cairo_paint(global_state.cairo);
        cairo_surface_destroy(s);
        cairo_restore(global_state.cairo);
        w = w->list.prev;
    }

    unlock(&global_state.render_lock);
}

static void set_fg_window(struct window *w)
{
    if (global_state.windows == w) {
        redraw(); // force redraw
        return;
    }
    lock(&global_state.render_lock);
    delete_from_list((void*)global_state.windows, w);
    add_to_list(global_state.windows, w);
    unlock(&global_state.render_lock);
    redraw();
    printf("active window set to %d\n", w->id);
}

static void mouse_click(int x, int y)
{
    FOR_EACH(w, global_state.windows, struct window) {
        if (in_rect(&w->w_rect, x, y) == true) {
            set_fg_window(w);
            printf("click event forwarded to window %d\n", w->id);
            break;
        }
    }
}

struct window *get_window(int id, int pid)
{
    FOR_EACH(w, global_state.windows, struct window) {
        if (w->id == id && w->pid == pid) {
            return w;
        }
    }

    return NULL;
}

static void close_window(struct window *w)
{
    lock(&global_state.render_lock);
    delete_from_list((void*)&global_state.windows, w);
    unlock(&global_state.render_lock);
    redraw();
    shm_unmap(w->buffers[0]);
    shm_unmap(w->buffers[1]);
    printf("window %d is deleted\n", w->id);
    free(w);
}

static struct window *create_window(size_t width, size_t height, uint32_t pid, uint32_t flags)
{
    struct window *w = malloc(sizeof(struct window));
    if (w == NULL) {
        printf("create_window failed: out of memory\n");
        exit(1);
    }
    memset(w, 0, sizeof(struct window));

    if (flags & WINDOW_FLAG_FULL_SCREEN) {
        width = global_state.surface_width;
        height = global_state.surface_height;
    }

    w->pid = pid;
    w->flags = flags;
    w->id = global_state.next_id++;
    w->w_rect.x = (int)(global_state.surface_width - width) / 2;
    w->w_rect.y = (int)(global_state.surface_height - height) / 2;
    w->w_rect.width = width;
    w->w_rect.height = height;

    if (flags & WINDOW_FLAG_FULL_SCREEN) {
        w->c_rect = w->w_rect;
    } else {
        /*w->c_rect.x = w->w_rect.x + WINDOW_BORDER_WIDTH;
        w->c_rect.y = w->w_rect.y + WINDOW_HEADER_HEIGHT;
        w->c_rect.width = w->w_rect.width - WINDOW_BORDER_WIDTH * 2;
        w->c_rect.height = w->w_rect.height - WINDOW_HEADER_HEIGHT - WINDOW_BORDER_WIDTH;*/
        w->c_rect = w->w_rect;
    }

    char name[20];
    sprintf(name, "mdm_surface_%i_%i", w->id, 0);
    shm_alloc(name, global_state.bbp * w->c_rect.width * w->c_rect.height);
    w->buffers[0] = shm_get_addr(name);
    sprintf(name, "mdm_surface_%i_%i", w->id, 1);
    shm_alloc(name, global_state.bbp * w->c_rect.width * w->c_rect.height);
    w->buffers[1] = shm_get_addr(name);

    lock(&global_state.render_lock);
    add_to_list(global_state.windows, w);
    unlock(&global_state.render_lock);
    printf("created window at %d:%d with size %d:%d for owner %d, id %d\n", w->w_rect.x, w->w_rect.y, w->w_rect.width, w->w_rect.height, w->pid, w->id);
    set_fg_window(w);
    return w;
}

void run_tests()
{
    struct window *w = create_window(200, 200, 12, 0);
    create_window(300, 400, 12, 0);
    set_fg_window(w);
    mouse_click(370, 270);
    mouse_click(370, 370);
    w = create_window(500, 800, 13, 0);
    close_window(w);
    mouse_click(350, 300);
}

int init_cairo()
{
    if (shm_map("system_fb")) {
        printf("can't map system framebuffer\n");
        return -1;
    }
    uintptr_t fb = shm_get_addr("system_fb");
    if (fb == NULL) {
        printf("can't get system framebuffer address\n");
        shm_unmap("system_fb");
        return -1;
    }

    global_state.fb = fb;

    int stride = cairo_format_stride_for_width (CAIRO_FORMAT_ARGB32, global_state.surface_width);
    global_state.cairo_surface = cairo_image_surface_create_for_data(
        fb, CAIRO_FORMAT_ARGB32, global_state.surface_width, global_state.surface_height, stride);
    global_state.cairo = cairo_create(global_state.cairo_surface);

    global_state.sprites.close_button = cairo_image_surface_create_from_png("etc/sprites/close.png");

    return 0;
}

void events_loop()
{
    int done = 0;
    struct event_mdm *packet = malloc(sizeof(struct event_mdm));
    while (true) {
        done = read(global_state.events_file, packet, sizeof(struct event_mdm));
        if (done > 0) {
            if (packet->param1 == REQUEST_CREATE_WINDOW) {
                struct window *w = create_window(packet->param2, packet->param3, packet->event.sender, packet->param4);
                packet->event.target = packet->event.sender;
                packet->event.sender = getpid();
                packet->param1 = RESPONSE_OK;
                packet->param2 = w->id;
                int debug = write(global_state.events_file, packet, sizeof(struct event_mdm));
            } else if (packet->param1 == REQUEST_CLOSE_WINDOW) {
                struct window *w = get_window(packet->param2, packet->event.sender);
                packet->event.target = packet->event.sender;
                packet->event.sender = getpid();
                packet->param1 = RESPONSE_OK;
                packet->param2 = (int)(w != NULL);
                close_window(w);
                write(global_state.events_file, packet, sizeof(struct event_mdm));
            } else if (packet->param1 == REQUEST_INVALIDATE_WINDOW) {
               struct window *w = get_window(packet->param2, packet->event.sender);
               packet->event.target = packet->event.sender;
               packet->event.sender = getpid();
               packet->param1 = RESPONSE_OK;
               //w->active_buffer = 1;
               redraw();
               write(global_state.events_file, packet, sizeof(struct event_mdm));
           }
        } else if (done == -1) {
            printf("read events error %i\n", errno);
        }
    }
}

int main(int argc, char *argv[])
{
    global_state.surface_width = 640;
    global_state.surface_height = 480;
    global_state.bbp = 32;
    global_state.next_id = 1;

    if (init_cairo()) {
        return 1;
    }

    //run_tests();
    global_state.events_file = open("/dev/event", O_RDWR);
    if (global_state.events_file  == -1) {
        printf("can't open events stream\n");
        return 1;
    }

    if (fork() == 0) {
        char *params[1];
        params[0] = 0;
        execve(argv[1], params, params);
    }

    events_loop();

    return 0;
}
