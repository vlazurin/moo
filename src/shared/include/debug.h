#ifndef H_DEBUG
#define H_DEBUG

#include <stdarg.h>

#define IN_KB(value) (value) / 1024
#define IN_MB(value) (value) / 1024 / 1024

void init_debug_serial();
void debug(const char *format, ...);
void breakpoint();

#ifdef DEBUG
#define assert(e, ...) ((e) ? (void)0 : debug("[assert] %s() in %s:%i. Expression: %s\n", __func__, __FILE__, __LINE__, #e ". " __VA_ARGS__))
#else
#define assert(e, ...) ((void)0)
#endif

#endif
