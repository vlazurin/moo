#include "../include/debug.h"
#include "../include/port.h"
#include "../include/string.h"
#include "../include/stdlib.h"

void breakpoint()
{
    asm("xchg %bx, %bx");
}

void _char_video(const char c)
{
    static uint8_t x = 0;
    static uint8_t y = 0;
    uint16_t *video = (uint16_t *)0xB8000 + y * 80 + x;
    if (c != '\n')
    {
        *video = c | (0x07 << 8);
        x++;
    }
    else
    {
        x = 0;
        y++;
    }

    if (x >= 80) {
        y++;
        x = 0;
    }
    if (y >= 25) {
        y = 0;
    }
}



void print_char(const char c)
{
    outb(0x0e9, c);
    while ((inb(0x3F8 + 5) & 0x20) == 0);
    outb(0x3F8, c);
}

void debug(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    char *str = 0;
    char buffer[11];
    memset(buffer, 0, 11);
    asm("cli");

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
                            print_char(*str);
                            str++;
                        }
                    break;
                    case 'h':
                        itoa(va_arg(args, uint32_t), buffer, 16);
                        str = (char*)&buffer;
                        print_char('0');
                        print_char('x');
                        while(*str != '\0')
                        {
                            print_char(*str);
                            str++;
                        }
                    break;
                    case 's':
                        str = va_arg(args, char*);
                        while(*str != '\0')
                        {
                            print_char(*str);
                            str++;
                        }
                    break;
                }
            break;
            default:
                print_char(*format);
            break;
        }
        format++;
    }

    asm("sti");
    va_end(args);
}
