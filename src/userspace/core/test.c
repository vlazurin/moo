#include <cairo/cairo.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#define NUM_GLYPHS 1
int shm_map(const char *name);
uintptr_t shm_get_addr(const char *name);
FT_Library  library;   /* handle to library     */
FT_Face     face;      /* handle to face object */
int main()
{
       cairo_status_t status;
int error = FT_Init_FreeType( &library );
error = FT_New_Face( library,
                     "/home/font.ttf",
                     0,
                     &face );
                     if (error) {
                         printf("cant load font\n");
                     }

    cairo_surface_t *surface;
    shm_map("system_fb");
    void *b = shm_get_addr("system_fb");
    cairo_t *cr;
    printf("1\n");
    int stride = cairo_format_stride_for_width (CAIRO_FORMAT_ARGB32, 640);
    surface = cairo_image_surface_create_for_data(b, CAIRO_FORMAT_ARGB32, 640, 480, stride);
    printf("2\n");
    cr = cairo_create (surface);
    printf("3\n");
    cairo_set_source_rgba (cr, 1, 0, 0, 1);
    cairo_move_to (cr, 0, 0);
    cairo_line_to (cr, 10, 10);

    printf("6\n");
cairo_font_face_t * cf =    cairo_ft_font_face_create_for_ft_face(face, 0);
printf("6.x\n");
    cairo_set_font_face(cr, cf);
printf("6.1\n");
  cairo_set_font_size(cr, 14);
printf("6.11\n");
  cairo_move_to(cr, 50, 50);
  printf("6.12\n");
  cairo_show_text(cr, "text...");
  cairo_move_to(cr, 100, 100);
  printf("6.12\n");
  cairo_show_text(cr, "text...");
  cairo_set_line_width(cr, 1);
  cairo_move_to (cr, 200, 200);
  cairo_line_to (cr, 300, 300);
  cairo_rectangle (cr, 500, 100, 50, 50);
  cairo_stroke(cr);
  printf("6.2\n");
    cairo_status_t xx = cairo_surface_write_to_png (surface, "/home/mol7.png");
    printf("%s\n", cairo_status_to_string (xx));
    printf("7\n");
    return 0;
}
