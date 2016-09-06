#ifndef H_SURFACE
#define H_SURFACE

#include <stdint.h>

typedef struct surface
{
    uint32_t *addr;
    uint32_t width;
    uint32_t height;
} surface_t;

#endif
