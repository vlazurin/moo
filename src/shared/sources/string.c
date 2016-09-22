#include "../include/string.h"
#include "../include/debug.h"

void *memset(void *s, int c, size_t n)
{
    assert(n > 0);
    unsigned char* p=s;
    while(n--)
        *p++ = (unsigned char)c;
    return s;
}

unsigned int strlen(const char *str)
{
    uint16_t size = 0;
    while(str[size] != '\0')
    {
        size++;
    }
    return size;
}

size_t strcmp(const char *str1, const char *str2, uint16_t count)
{
    size_t res = 0;
    uint16_t cnt = 0;
    while (!(res = *(unsigned char*)str1 - *(unsigned char*)str2) && cnt < count - 1)
    {
        ++str1;
        ++str2;
        cnt++;
    }
    return res;
}

void memcpy(void *dest, const void *src, size_t n)
{
    char *dp = dest;
    const char *sp = src;
    while (n--)
    {
        *dp++ = *sp++;
    }
}

void strlower(char *str, size_t n)
{
    while(n > 0)
    {
        if (*str >= 65 && *str <= 90)
        {
            *str += 32;
        }
        str++;
        n--;
    }
}

char *trim(char *str)
{
    char *end;

    // Trim leading space
    while(*str == ' ') str++;

    if(*str == 0)  // All spaces?
        return str;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while(end > str && *end == ' ') end--;

    // Write new null terminator
    *(end + 1) = 0;

    return str;
}

int strpos(char *str, char c)
{
    int number = 0;
    while(*str != '\0')
    {
       if (c == *str)
        {
            return number;
        }

        number++;
        str++;
    }

    return -1;
}
