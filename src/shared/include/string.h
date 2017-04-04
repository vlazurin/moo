#ifndef H_STRING
#define H_STRING

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

void *memset(void *str, int c, size_t n);
unsigned int strlen(const char *str);
size_t strcmp(const char *s1, const char *s2);
int strncmp(const char * s1, const char * s2, size_t n);
char* strtok_r(char *str, const char *delim, char **nextp);
size_t strspn(const char *s1, const char *s2);
char *strchr(const char *s, int c);
size_t strcspn(const char *s1, const char *s2);
void memcpy(void *dest, const void *src, size_t n);
void strlower(char *str, size_t n);
void strup(char *str, size_t n);
char *trim(char *str);
int strpos(const char *str, char c);
char *strrchr(char *s, int ch);
void strcpy(char *s1, char *s2);
void sprintf(char *res, const char *format, ...);

#endif
