#ifndef H_STDLIB
#define H_STDLIB

#include <stdint.h>

char *itoa(int value, char *str, int base);
char *utoa(uint32_t value, char *str, int base);
int atoi(char *str);

#endif
