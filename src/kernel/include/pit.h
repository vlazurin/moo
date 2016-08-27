#ifndef H_PIT
#define H_PIT

#include <stdint.h>

#define PIT_FREQUENCY 1193182
#define TICK_FREQUENCY 1193

#define PIT_COUNTER_0 0x40
#define PIT_CMD 0x43

#define CMD_BINARY 0x00
#define CMD_MODE_2 0x04 // Rate Generator
#define CMD_RW_BOTH 0x30 // Least followed by Most Significant Byte
#define CMD_COUNTER_0 0x00

void init_pit();
uint32_t get_pit_ticks();

#endif
