#include "../include/system.h"

gdt_entry_t gdt_entries[GDT_SIZE];
gdt_register_t gdt_reg;

void gdt_set_gate(uint16_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t granularity)
{
    gdt_entries[num].base_low = (base & 0xFFFF);
    gdt_entries[num].base_middle = (base >> 16) & 0xFF;
    gdt_entries[num].base_high = (base >> 24) & 0xFF;

    gdt_entries[num].limit_low = (limit & 0xFFFF);
    gdt_entries[num].granularity = ((limit >> 16) & 0x0F);

    gdt_entries[num].granularity |= (granularity & 0xF0);
    gdt_entries[num].access = access;
}

void gdt_init()
{
    gdt_reg.limit = (sizeof(gdt_entry_t) * GDT_SIZE) - 1;
    gdt_reg.base = (uint32_t)&gdt_entries;
}

void set_gdt()
{
    asm("lgdt gdt_reg");
}

uint16_t crc16(uint8_t *data, uint32_t length)
{
    uint8_t x;
    uint16_t crc = 0xFFFF;

    while (length--){
        x = crc >> 8 ^ *data++;
        x ^= x>>4;
        crc = (crc << 8) ^ ((uint16_t)(x << 12)) ^ ((uint16_t)(x <<5)) ^ ((uint16_t)x);
    }
    return crc;
}

void hlt()
{
    __asm("cli;hlt;");
}
