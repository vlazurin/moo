#include "../include/string.h"
#include "../include/stdlib.h"

void strcpy(char *s1, char *s2)
{
    memcpy(s1, s2, strlen(s2) + 1); // +1 because we need to copy '\0'
}

char *strrchr(char *s, int ch)
{
    char *save = 0;
    while(*s != '\0')
    {
        if (*s == (char)ch)
        {
            save = s;
        }
        s++;
    }

    return save;
}

void *memset(void *s, int c, size_t n)
{
    unsigned char* p=s;
    while(n--)
        *p++ = (unsigned char)c;
    return s;
}

char *strchr(const char *s, int c)
{
    while (*s != (char)c)
        if (!*s++)
            return 0;
    return (char *)s;
}

size_t strspn(const char *s1, const char *s2)
{
    size_t ret=0;
    while(*s1 && strchr(s2,*s1++))
        ret++;
    return ret;
}

size_t strcspn(const char *s1, const char *s2)
{
    size_t ret=0;
    while(*s1)
        if(strchr(s2,*s1))
            return ret;
        else
            s1++,ret++;
    return ret;
}

/*
 * public domain strtok_r() by Charlie Gordon
 *
 *   from comp.lang.c  9/14/2007
 *
 *      http://groups.google.com/group/comp.lang.c/msg/2ab1ecbb86646684
 *
 *     (Declaration that it's public domain):
 *      http://groups.google.com/group/comp.lang.c/msg/7c7b39328fefab9c
 */

char* strtok_r(char *str, const char *delim, char **nextp)
{
    char *ret;

    if (str == NULL)
    {
        str = *nextp;
    }

    str += strspn(str, delim);

    if (*str == '\0')
    {
        return NULL;
    }

    ret = str;

    str += strcspn(str, delim);

    if (*str)
    {
        *str++ = '\0';
    }

    *nextp = str;

    return ret;
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

size_t strcmp(const char *s1, const char *s2)
{
    while(*s1 && (*s1==*s2))
    {
        s1++,
        s2++;
    }
    return *(const unsigned char*)s1-*(const unsigned char*)s2;
}

int strncmp(const char* s1, const char* s2, size_t n)
{
    while(n--)
        if(*s1++!=*s2++)
            return *(unsigned char*)(s1 - 1) - *(unsigned char*)(s2 - 1);
    return 0;
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

void strup(char *str, size_t n)
{
    while(n > 0) {
        if (*str >= 97 && *str <= 122) {
            *str -= 32;
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

int strpos(const char *str, char c)
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

void sprintf(char *res, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    char *str = 0;
    char buffer[15];
    memset(buffer, 0, 15);

    while(*format != '\0')
    {
        switch(*format)
        {
            case '%':
                format++;
                switch((char)*format)
                {
                    case 'i':
                        itoa(va_arg(args, uint32_t), buffer, 10);
                        str = (char*)&buffer;
                        while(*str != '\0')
                        {
                            *res++ = *str++;
                        }
                    break;
                    case 'h':
                        itoa(va_arg(args, uint32_t), buffer, 16);
                        str = (char*)&buffer;
                        *res++ = '0';
                        *res++ = 'x';
                        while(*str != '\0')
                        {
                            *res++ = *str++;
                        }
                    break;
                    case 's':
                        str = va_arg(args, char*);
                        while(*str != '\0')
                        {
                            *res++ = *str++;
                        }
                    break;
                }
            break;
            default:
                *res++ = *format;
            break;
        }
        format++;
    }
    *res = '\0';
    va_end(args);
}
