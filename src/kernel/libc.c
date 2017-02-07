#include "libc.h"
#include "string.h"
#include "liballoc.h"
#include <stdint.h>

char *strdup(char *str)
{
    uint32_t len = strlen(str) + 1; // +1 because '\0' must be copied too
    char *copy = kmalloc(len);
    memcpy(copy, str, len);
    return copy;
}
