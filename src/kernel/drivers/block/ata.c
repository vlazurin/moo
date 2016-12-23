/*__asm__(".code32\n");
#include "../include/ata.h"
#include <stdint.h>
#define SECTOR_SIZE = 512;

uint8_t inportb(uint16_t portid);
void outportb(uint16_t portid, uint8_t value);
uint16_t inportw(uint16_t portid);

void delay(io)
{
    int i;
    for(i = 0; i < 4; i++)
    {
        inportb(io + ATA_PORT_COMMAND);
    }
}

void ata_read(uint8_t *buffer, uint16_t start_sector, uint8_t sectors_count)
{
    outportb(0x1F1, 0x00);
    outportb(0x1F2, sectors_count);
    outportb(0x1F3, (unsigned char)start_sector);
    outportb(0x1F4, (unsigned char)(start_sector>>8));
    outportb(0x1F5, (unsigned char)(start_sector>>16));
    outportb(0x1F6, (uint8_t)(0xE0 | (0xA0 << 4) | ((0 >> 24) & 0x0F)));
    outportb(0x1F7, 0x20);

    while (!(inportb(0x1F7) & 0x08));

    uint16_t i = 0, tmp;
    for (; i < 256 * sectors_count; i++)
    {
        tmp = inportw(0x1F0);
        buffer[i * 2] = (uint8_t) tmp;
        buffer[i * 2 + 1] = (uint8_t)(tmp >> 8);
    }
}
*/

#include "ata.h"
#include "debug.h"

struct ide_channel channels[2];

void init_ata(pci_device_t *pci_device)
{

}
