#include "gui.h"
#include "mdm.h"
#include "event.h"
#include "stddef.h"
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

int redraw_window(uint32_t id)
{
    struct event_mdm *packet = malloc(sizeof(struct event_mdm));
    memset(packet, 0, sizeof(struct event_mdm));
    packet->event.event_type = EVENT_TYPE_CUSTOM;
    packet->event.target = 1; // TODO: get it from env?
    packet->event.size = sizeof(struct event_mdm);
    packet->param1 = MDM_REQUEST_INVALIDATE_WINDOW;
    packet->param2 = id;

    int errno = write(event_stream, packet, sizeof(struct event_mdm));
    if (errno == -1 || errno != sizeof(struct event_mdm)) {
        printf("ops, invalidate_window is broken: can't write event, errno = %i\n", errno);
        free(packet);
        return -1;
    }

    int done = 0;
    while (true) {
        done = read(event_stream, packet, sizeof(struct event_mdm));
        if (done == -1) {
            printf("ops, invalidate_window is broken: can't read events, errno = %i\n", errno);
            break;
        }
        if (done == 0) {
            continue;
        }
        if (packet->event.sender != 1) // TODO: get it from env?
        {
            // TODO: move me to events list for handling
        }
        if (packet->param1 != RESPONSE_OK) {
            printf("ops, invalidate_window is broken: mdm response isn't OK, param1 = %i\n", packet->param1);
            break;
        }
        int id = packet->param2;
        free(packet);
        return id;
    }

    free(packet);
    return -1;
}

int create_window(size_t width, size_t height, uint32_t flags)
{
    struct event_mdm *packet = malloc(sizeof(struct event_mdm));
    memset(packet, 0, sizeof(struct event_mdm));
    packet->event.event_type = EVENT_TYPE_CUSTOM;
    packet->event.target = 1; // TODO: get it from env?
    packet->event.size = sizeof(struct event_mdm);
    packet->param1 = MDM_REQUEST_CREATE_WINDOW;
    packet->param2 = width;
    packet->param3 = height;
    packet->param4 = flags;

    int errno = write(event_stream, packet, sizeof(struct event_mdm));
    if (errno == -1 || errno != sizeof(struct event_mdm)) {
        printf("ops, create_window is broken: can't write event, errno = %i\n", errno);
        free(packet);
        return -1;
    }

    int done = 0;
    memset(packet, 0, sizeof(struct event_mdm));
    while (true) {
        done = read(event_stream, packet, sizeof(struct event_mdm));
        if (done == -1) {
            printf("ops, create_window is broken: can't read events, errno = %i\n", errno);
            break;
        }
        if (done == 0) {
            continue;
        }
        if (packet->event.sender != 1) // TODO: get it from env?
        {
            // TODO: move me to events list for handling
        }
        if (packet->param1 != RESPONSE_OK) {
            printf("ops, create_window is broken: mdm response isn't OK, param1 = %i\n", packet->param1);
            break;
        }
        int id = packet->param2;
        free(packet);
        return id;
    }

    free(packet);
    return -1;
}

void draw_label(struct context* ctx, struct label *l)
{
    /*cairo_set_source_rgba(global_state.cairo, RED(l->control->background_color) / 255,
        GREEN(l->control->background_color) / 255, BLUE(l->control->background_color) / 255, ALPHA(l->control->background_color) / 255);
    cairo_rectangle(global_state.cairo, 0, 0, global_state.surface_width, global_state.surface_height);*/
}

struct label *create_label()
{
    /*struct label *l = malloc(sizeof(struct label));
    memset(l, 0, sizeof(struct label));
    l->control.background_color = ARGB(255, 255, 255, 255);
    l->control.border_color = ARGB(255, 0, 0, 0);
    l->control.draw = &draw_label;
    return l;*/
}
