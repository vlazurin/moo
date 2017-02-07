#include "log.h"
#include "port.h"
#include "string.h"
#include "stdlib.h"
#include "log.h"
#include "mutex.h"

#define EARLY_COM_PORT_ADDR 0x3F8
#define MAX_STR_LENGTH 1024

#define early_write(message) \
early_write_e9(message); \
early_write_com(message);

static int log_level = KERN_DEBUG;
static mutex_t mutex = 0;
static char message[MAX_STR_LENGTH];
static char *labels[] = {
    "[FATAL] ",
    "[ERROR] ",
    "[WARN ] ",
    "[INFO ] ",
    "[DEBUG] "
};

/*
 * Function must be used only when kernel isn't fully initialized.
 * Currently i have no ideas how to write logs to file... log() call in VFS write() will cause infinity loop
 */
static void early_write_e9(const char *str)
{
    while(*str != '\0') {
        outb(0x0e9, *str);
        str++;
    }
}

/*
 * Function must be used only when kernel isn't fully initialized.
 * Currently i have no ideas how to write logs to file... log() call in VFS write() will cause infinity loop
 */
static void early_write_com(const char *str)
{
    while(*str != '\0') {
        while ((inb(EARLY_COM_PORT_ADDR + 5) & 0x20) == 0);
        outb(EARLY_COM_PORT_ADDR, *str);
        str++;
    }
}

void init_early_log()
{
    outb(EARLY_COM_PORT_ADDR + 1, 0x00);    // Disable all interrupts
    outb(EARLY_COM_PORT_ADDR + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(EARLY_COM_PORT_ADDR + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    outb(EARLY_COM_PORT_ADDR + 1, 0x00);    //                  (hi byte)
    outb(EARLY_COM_PORT_ADDR + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(EARLY_COM_PORT_ADDR + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    outb(EARLY_COM_PORT_ADDR + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

void log(int level, const char *format, ...)
{
    if (level > log_level || level < 0 || level > KERN_DEBUG) {
        return;
    }

    va_list args;
    va_start(args, format);

    mutex_lock(&mutex);

    early_write(labels[level]);

    char *ptr = message;
    uint32_t message_size = 0;
    char buf[11];
    uint32_t len;
    char *str;

    while(*format != '\0') {
        str = NULL;
        switch(*format) {
            case '%':
                format++;
                switch(*format) {
                    case 'i':
                    case 'd':
                        // 32 bits integer needs maximum 10 chars
                        memset(buf, 0, 11);
                        itoa(va_arg(args, int), buf, 10);
                        str = buf;
                    break;
                    case 'u':
                        // 32 bits integer needs maximum 10 chars
                        memset(buf, 0, 11);
                        utoa(va_arg(args, uint32_t), buf, 10);
                        str = buf;
                    break;
                    case 'x':
                        // 32 bits integer in hex format needs maximum 8 chars + 2 chars for "0x"
                        memset(buf, 0, 11);
                        buf[0] = '0';
                        buf[1] = 'x';
                        utoa(va_arg(args, uint32_t), &buf[2], 16);
                        str = buf;
                    break;
                    case 's':
                        str = va_arg(args, char*);
                    break;
                }
            break;
            default:
                buf[0] = *format;
                buf[1] = '\0';
                str = buf;
            break;
        }

        if (str != NULL) {
            len = strlen(str);
            if (message_size + len >= MAX_STR_LENGTH - 1) { // -1 because of EOS char
                early_write(labels[KERN_INFO]);
                early_write("write log error: message is too long\n");
                return;
            }
            memcpy(ptr, str, len);
            ptr += len;
            message_size += len;
        }
        format++;
    }
    *ptr = '\0';
    early_write(message);

    mutex_release(&mutex);
    va_end(args);
}
