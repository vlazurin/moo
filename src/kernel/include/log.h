#ifndef LOG_H
#define LOG_H

#include <stdarg.h>
#include "system.h"

#define IN_KB(value) (value) / 1024
#define IN_MB(value) (value) / 1024 / 1024

#define KERN_FATAL 0
#define KERN_ERR 1
#define KERN_WARNING 2
#define KERN_INFO 3
#define KERN_DEBUG 4

#define debug(...) log(KERN_DEBUG, __VA_ARGS__)

#define assert_kspace(e) (assert((uint32_t)e >= KERNEL_SPACE_ADDR))

#ifdef DEBUG
#define assert(e) ((e) ? (void)0 : log(KERN_INFO, "assert failed: %s:%s():%i. Expression: %s\n", __FILE__, __func__, __LINE__, #e))
#else
#define assert(e) ((void)0)
#endif

void log(int level, const char *format, ...);
void init_early_log();

#endif
