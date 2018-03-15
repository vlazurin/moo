#ifndef H_PRIMITIVES
#define H_PRIMITIVES

#include <stdint.h>
#include <stddef.h>

struct rect {
    int x;
    int y;
    size_t width;
    size_t height;
};

struct size {
    int width;
    int height;
};

#endif
