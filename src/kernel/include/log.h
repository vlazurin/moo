#ifndef LOG_H
#define LOG_H

#include <stdarg.h>

#define IN_KB(value) (value) / 1024
#define IN_MB(value) (value) / 1024 / 1024

#define KERN_EMERG 0
#define KERN_ERR 1
#define KERN_WARNING 2
#define KERN_INFO 3
#define KERN_DEBUG 4

void log(int level, const char *format, ...);
void init_log();

#endif
