#include "screen.h"
#include "debug.h"
#include "vfs.h"
#include "string.h"

#define VIDEO_MEM 0xB8000
#define VIDEO_SIZE (80 * 25)
#define DEFAULT_COLOR 0x07

uint32_t x;
uint32_t y;

static int screen_write(vfs_file_t *file, void *buf, uint32_t size, uint32_t *offset)
{
    char *str = buf;
    uint32_t count = 0;

    while(count < size) {
        if (*str == '\n') {
            x = 0;
            if (y == 24) {
                memcpy((char *)VIDEO_MEM, (char *)VIDEO_MEM + 80 * 2, 80 * 24 * 2);
                memset((char *)VIDEO_MEM + VIDEO_SIZE * 2 - 80 * 2, 0, 80 * 2);
            } else {
                y++;
            }
        } else {
            char *mem = (char *)VIDEO_MEM + (80 * y + x) * 2;
            *mem++ = *str;
            *mem++ = DEFAULT_COLOR;
            if (x == 79) {
                x = 0;
                if (y == 24) {
                    memcpy((char *)VIDEO_MEM, (char *)VIDEO_MEM + 80 * 2, 80 * 24 * 2);
                    memset((char *)VIDEO_MEM + VIDEO_SIZE * 2 - 80 * 2, 0, 80 * 2);
                } else {
                    y++;
                }
            } else {
                x++;
            }
        }
        count++;
        str++;
    }
    return count;
}

static vfs_file_operations_t screen_file_ops = {
    .open = 0,
    .close = 0,
    .write = &screen_write,
    .read = 0
};

void init_screen()
{
    char *mem = (char *)VIDEO_MEM;
    for(uint32_t i = 0; i < VIDEO_SIZE * 2; i++) {
        *mem++ = 0;
    }

    x = 0;
    y = 0;
    create_vfs_node("/dev/screen", S_IFCHR, &screen_file_ops, 0, 0);
}
