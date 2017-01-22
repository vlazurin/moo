#include "../include/system.h"

void gdt_set_gate(struct gdt_entry *entries, uint16_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t granularity)
{
    entries[num].base_low = (base & 0xFFFF);
    entries[num].base_middle = (base >> 16) & 0xFF;
    entries[num].base_high = (base >> 24) & 0xFF;

    entries[num].limit_low = (limit & 0xFFFF);
    entries[num].granularity = ((limit >> 16) & 0x0F);

    entries[num].granularity |= (granularity & 0xF0);
    entries[num].access = access;
}

uint16_t crc16(uint8_t *data, uint32_t length)
{
    uint8_t x = 0;
    uint16_t crc = 0xFFFF;

    while (length--) {
        x = crc >> 8 ^ *data++;
        x ^= x >> 4;
        crc = (crc << 8) ^ ((uint16_t)(x << 12)) ^ ((uint16_t)(x <<5)) ^ ((uint16_t)x);
    }
    return crc;
}

void hlt()
{
    __asm("cli;hlt;");
}
