#include "string.h"
#include "liballoc.h"

#define STBTT_memcpy       memcpy
#define STBTT_memset       memset
#define STBTT_assert(x)    ((void)0)
#define STBTT_strlen(x)    strlen(x)

#define STBTT_sqrt(x)      sqrt(x)
#define STBTT_ifloor(x)    ((int)i_floor(x))
#define STBTT_iceil(x)     ((int)i_ceil(x))
#define STBTT_malloc(x,u)  ((void)(u), kmalloc(x))
#define STBTT_free(x,u)    ((void)(u), kfree(x))
#define STBTT_fabs(x)      fabs(x)

double sqrt(double x)
{
    return 0;
}

double fabs(double x)
{
    return  x < 0 ? -x : x;
}

int i_floor(float x)
{
    return (int)x;
}

int i_ceil(float x)
{
    int z;
    z = x;
    if (z > x)
    {
        return z;
    }
    else
    {
        return (z + 1);
    }
}

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
