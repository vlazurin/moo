#ifndef H_PORT
#define H_PORT

#include <stdint.h>

void outb(uint16_t port_id, uint8_t value);
void outw(uint16_t port_id, uint16_t value);
void outl(uint16_t port_id, uint32_t value);

uint32_t inl(uint16_t port_id);
uint16_t inw(uint16_t port_id);
uint8_t inb(uint16_t port_id);

#endif
