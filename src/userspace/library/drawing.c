#include "drawing.h"

uint32_t premultiply(uint32_t color)
{
	uint16_t a = ALPHA(color);
	uint16_t r = RED(color) * a / 255;
	uint16_t g = GREEN(color) * a / 255;
	uint16_t b = BLUE(color) * a / 255;

	return ARGB(a, r, g, b);
}

uint8_t in_rect(struct rect *r, int x, int y)
{
    return (x >= r->x && x < r->x + r->width && y >= r->y && y < r->y + r->height);
}
