#include "../include/port.h"

void outb(uint16_t port_id, uint8_t value)
{
    asm volatile("outb %%al, %%dx" :: "d"(port_id), "a"(value));
}

void outw(uint16_t port_id, uint16_t value)
{
    asm volatile("outw %%ax, %%dx" :: "d"(port_id), "a"(value));
}

void outl(uint16_t port_id, uint32_t value)
{
    asm volatile("outl %%eax, %%dx" :: "d"(port_id), "a"(value));
}

uint32_t inl(uint16_t port_id)
{
    uint32_t ret;
    asm volatile("inl %%dx, %%eax":"=a"(ret):"d"(port_id));
    return ret;
}

uint16_t inw(uint16_t port_id)
{
    uint16_t ret;
    asm volatile("inw %%dx, %%ax":"=a"(ret):"d"(port_id));
    return ret;
}

uint8_t inb(uint16_t port_id)
{
    uint8_t ret;
    asm volatile("inb %%dx, %%al":"=a"(ret):"d"(port_id));
    return ret;
}
