#ifndef H_BIOS
#define H_BIOS

#include <stdint.h>
#include "system.h"

typedef struct lba_disk_address_packet
{
    uint8_t size;
    uint8_t reserved;
    uint16_t sectors_count;
    uint16_t buffer_offset;
    uint16_t buffer_segment;
    uint32_t lba_address;
    uint32_t lba_address_upper;
} __attribute__((packed, aligned (4))) lba_disk_address_packet_t;

void read_lba(uint32_t lba_address, uint16_t sectors_count, void *buffer);
void print(const char *str);
uint8_t detect_upper_memory(memory_map_entry_t *memory_map);

#endif
