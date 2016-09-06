#ifndef H_MMIO
#define H_MMIO

#include <stdint.h>

inline __attribute__((always_inline)) void mmio_write32(uint32_t base, uint32_t offset, uint32_t value) 
{
    *(volatile uint32_t *)(base + offset) = value;
}

inline __attribute__((always_inline)) uint32_t mmio_read32(uint32_t base, uint32_t offset)
{
    return *(volatile uint32_t *)(base + offset);
}

#endif
