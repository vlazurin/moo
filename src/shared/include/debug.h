#ifndef H_DEBUG
#define H_DEBUG

#include <stdarg.h>

#define IN_KB(value) (value) / 1024
#define IN_MB(value) (value) / 1024 / 1024

void debug(const char *format, ...);
void breakpoint();

#endif
