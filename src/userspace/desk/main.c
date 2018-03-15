#include "gui.h"
#include "text.h"
#include <string.h>
#include <stdbool.h>
#include <cairo/cairo.h>

int shm_map(char *);
uintptr_t* shm_get_addr(char *);
extern int errno;

cairo_surface_t *cairo_surface;
cairo_t *cairo;
cairo_surface_t *background;
uint32_t window_id;
uintptr_t raw_surface;

char *grid;

void init_text();

void draw()
{
    cairo_set_source_surface(cairo, background, 0, 0);
    cairo_paint(cairo);
    int ch = 480 / FONT_HEIGHT;
    for(int i = 0; i < ch; i++) {
        draw_text("hello", (void*)raw_surface, 0, i * FONT_HEIGHT, 5);
    }
    redraw_window(window_id);
}

void event_loop()
{
    while(true)
    {

    }
}

int main(int argc, char *argv[])
{
    init_gui();

    window_id = create_window(640, 480, WINDOW_FLAG_ALWAYS_VISIBLE);
    if (window_id < 0) {
        printf("Can't create window\n");
        return 1;
    }

    char name[20];
    sprintf(name, "mdm_surface_%i_0", window_id);
    if (shm_map(name)) {
        printf("Can't map surface mdm_surface_%i_0\n", window_id);
        return 1;
    }
    uintptr_t addr = shm_get_addr(name);

    int stride = cairo_format_stride_for_width (CAIRO_FORMAT_ARGB32, 640);
    cairo_surface = cairo_image_surface_create_for_data(
        (void*)addr, CAIRO_FORMAT_ARGB32, 640, 480, stride);
    cairo = cairo_create(cairo_surface);

    raw_surface = addr;

    init_text();

    int w = 640 / FONT_WIDTH;
    int h = 480 / FONT_HEIGHT;
    grid = malloc(w * h);
    memset(grid, 0, w * h);

    background = cairo_image_surface_create_from_png("/etc/sprites/b_big.png");

    //initial redraw
    draw();
    // TODO: possibly app can be DDOSed because of delay between window creation and event_loop call
    event_loop();

    cairo_surface_destroy(background);
}
