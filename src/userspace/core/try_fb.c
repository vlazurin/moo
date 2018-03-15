#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

int shm_map(const char *name);
uintptr_t shm_get_addr(const char *name);

int main()
{
    shm_map("system_fb");
    uint32_t *ptr = shm_get_addr("system_fb");
    *ptr = 0x00FF00FF;
    *(ptr+1) = 0x00FF0000;
    *(ptr+2) = 0x0000FF00;
    return 0;
}
