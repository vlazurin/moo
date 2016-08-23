#include <stdint.h>
#include "../include/bios.h"

#define CPU_CF_FLAG (1 << 0)

void read_lba(uint32_t lba_address, uint16_t sectors_count, void *buffer)
{
    uint16_t offset = 0;
    uint16_t segment = 0;

    // pointers size is 4 bytes even we are in real mode
    if ((uintptr_t)buffer >= 0x100000)
    {
        print("LBA buffer must be in the first 1 mb of memory");
        hlt();
    }

    if ((uintptr_t)buffer % 2 != 0)
    {
        print("LBA buffer must be aligned on 2 bytes boundary");
        hlt();
    }

    if ((uintptr_t)buffer >= 0x10000)
    {
        segment = (uintptr_t)buffer / 0x10000 * 0x1000;
        offset = (uintptr_t)buffer % 0x10000;
    }
    else
    {
        offset = (uintptr_t)buffer;
    }

    lba_disk_address_packet_t packet;
    packet.size = sizeof(lba_disk_address_packet_t);
    packet.reserved = 0;
    packet.lba_address_upper = 0;
    packet.lba_address = lba_address;
    packet.sectors_count = sectors_count;
    packet.buffer_offset = offset;
    packet.buffer_segment = segment;

    uint16_t flags = 0;
    uint8_t code = 0;

    asm("pusha");
    asm("movw %0, %%si" :: "r"((uint16_t)(intptr_t)&packet));
    asm("movb $0x42, %ah");
    asm("movb $0x80, %dl");
    asm("int $0x13");
    asm("movb %%ah, %0" : "=r"(code));
    asm("pushf");
    // flags register size is 16 bit, but in stack it takes 4 bytes, so popl instruction must be used
    asm("popl %eax");
    asm("movw %%ax, %0" : "=r"(flags));
    asm("popa");

    // LBA read isn't success if CF flag is set or code isn't 0
    if (flags & CPU_CF_FLAG || code > 0)
    {
        print("LBA read error");
        hlt();
    }
}
