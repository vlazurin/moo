#ifndef H_DRAWING
#define H_DRAWING

#include <stdint.h>
#include "primitives.h"

#define ALPHA(c) ((c & 0xFF000000) >> 24)
#define RED(c) ((c & 0x00FF0000) >> 16)
#define GREEN(c) ((c & 0x0000FF00) >> 8)
#define BLUE(c) (c & 0x000000FF)
#define ARGB(a, r, g, b) (((a & 0x000000FF) << 24) | ((r & 0x000000FF) << 16) | ((g & 0x000000FF) << 8) | (b & 0x000000FF))

uint32_t premultiply(uint32_t color);
uint8_t in_rect(struct rect *r, int x, int y);

#endif
