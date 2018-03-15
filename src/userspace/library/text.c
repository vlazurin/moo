#include "text.h"
#include "drawing.h"
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include FT_FREETYPE_H

FT_Library library;
FT_Face face;

void init_text()
{
    int error = FT_Init_FreeType(&library);
    if (error) {
        printf("Freetype initialization failed\n");
        exit(1);
    }

    error = FT_New_Face(library, "/etc/system.ttf", 0, &face);
    if (error) {
        printf("Freetype load system font failed\n");
        exit(1);
    }

    error = FT_Set_Pixel_Sizes(face, FONT_SIZE, FONT_SIZE);
    if (error) {
        printf("Freetype set pixel size failed\n");
        exit(1);
    }
}
static inline uint16_t min16(uint16_t a, uint16_t b) {
	return (a < b) ? a : b;
}
#define SGFX(CTX,x,y,WIDTH) *((uint32_t *)&CTX[((WIDTH) * (y) + (x)) * 4])

#define DONT_USE_FLOAT_FORALPHAHA 1

uint32_t alpha_blend_rgba(uint32_t bottom, uint32_t top) {
	if (ALPHA(bottom) == 0) return top;
	if (ALPHA(top) == 255) return top;
	if (ALPHA(top) == 0) return bottom;
#if DONT_USE_FLOAT_FORALPHAHA
	uint16_t a = ALPHA(top);
	uint16_t c = 255 - a;
	uint16_t b = ((int)ALPHA(bottom) * c) / 255;
	uint16_t alp = min16(a + b, 255);
	uint16_t red = min16((uint32_t)(RED(bottom) * c + RED(top) * 255) / 255, 255);
	uint16_t gre = min16((uint32_t)(GREEN(bottom) * c + GREEN(top) * 255) / 255, 255);
	uint16_t blu = min16((uint32_t)(BLUE(bottom) * c + BLUE(top) * 255) / 255, 255);
	return ARGB(alp, red,gre,blu);
#else
	double a = ALPHA(top) / 255.0;
	double c = 1.0 - a;
	double b = (ALPHA(bottom) / 255.0) * c;
	double alp = a + b; if (alp > 1.0) alp = 1.0;
	double red = (RED(bottom) / 255.0) * c + (RED(top) / 255.0); if (red > 1.0) red = 1.0;
	double gre = (GREEN(bottom) / 255.0) * c + (GREEN(top) / 255.0); if (gre > 1.0) gre = 1.0;
	double blu = (BLUE(bottom) / 255.0) * c + (BLUE(top) / 255.0); if (blu > 1.0) blu = 1.0;
	return ARGB( alp * 255, red * 255, gre * 255, blu * 255);
#endif
}

static void draw_char(FT_Bitmap * bitmap, int x, int y, uint32_t fg, void *ctx) {
	int i, j, p, q;
	int x_max = x + bitmap->width;
	int y_max = y + bitmap->rows;
	for (j = y, q = 0; j < y_max; j++, q++) {
		if (j < 0 || j >= 480) continue;
		for ( i = x, p = 0; i < x_max; i++, p++) {
			uint32_t a = ALPHA(fg);
			a = (a * bitmap->buffer[q * bitmap->width + p]) / 255;
			uint32_t tmp = premultiply(ARGB(a, RED(fg),GREEN(fg),BLUE(fg)));
			if (i < 0 || i >= 640) continue;
			SGFX(ctx,i,j,640) = alpha_blend_rgba(SGFX(ctx,i,j,640),tmp);
		}
	}
}

void draw_text(const char* str, void* surface, int x, int y, int num_chars)
{
    int pen_x = x;
    int pen_y = y;
    for ( int n = 0; n < num_chars; n++ )
    {
        FT_UInt glyph_index;
        glyph_index = FT_Get_Char_Index(face, str[n]);
        int error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
        if (error) {
            printf("Freetype load glyph failed\n");
            exit(1);
        }

        error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
        if (error) {
            printf("Freetype render glyph failed\n");
            exit(1);
        }
FT_GlyphSlot  slot = face->glyph;

  draw_char(&slot->bitmap,
                  pen_x + slot->bitmap_left,
                  pen_y - slot->bitmap_top, 0xFF0000FF, surface);

        pen_x += face->glyph->advance.x >> 6;
        pen_y += face->glyph->advance.y >> 6;
    }
}
