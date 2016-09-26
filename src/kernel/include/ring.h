#ifndef H_RING
#define H_RING

#include <stdint.h>
#include <mutex.h>

typedef struct ring
{
    uint32_t *buffer;
    uint32_t head;
    uint32_t tail;
    uint32_t size;
    mutex_t head_mutex;
    mutex_t tail_mutex;

    uint8_t (*push)(struct ring*, void*);
    void* (*pop)(struct ring*);
    void (*free)(struct ring*);
} ring_t;

enum
{
    RING_BUFFER_OK = 1,
    RING_BUFFER_FULL
};

ring_t* create_ring(uint32_t size);

#endif
