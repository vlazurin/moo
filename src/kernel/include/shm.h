#ifndef H_SHM
#define H_SHM

#include "list.h"
#include <stddef.h>

#define SHM_SEGMENT_NAME_LENGTH 128

struct shm_segment
{
    struct list_node list;
    char name[SHM_SEGMENT_NAME_LENGTH + 1]; // +1 for EOS char
    size_t size;
    size_t pages_count;
    int ref_count;
    uint8_t persistent;
    uintptr_t *pages;
};

struct shm_map
{
    struct list_node list;
    uintptr_t addr;
    struct shm_segment *segment;
};

int shm_alloc(const char *name, uint32_t size, uintptr_t *pages, uint8_t persistent);
int shm_map(const char *name);
int shm_unmap(const char *name);
uint32_t shm_get_addr(const char *name);
void init_shm();

#endif
