#ifndef H_STRING
#define H_STRING

#include <stdint.h>
#include <stddef.h>

void *memset(void *str, int c, size_t n);
unsigned int strlen(const char *str);
size_t strcmp(const char *str1, const char *str2, uint16_t count);
void memcpy(void *dest, const void *src, size_t n);
void strlower(char *str, size_t n);
char *trim(char *str);
int strpos(char *str, char c);

#endif
