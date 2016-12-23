#ifndef H_BUFFER
#define H_BUFFER

#include <stdint.h>
#include "mutex.h"

enum
{
    BUFFER_OK = 1,
    BUFFER_FULL
};

typedef struct buffer
{
    uint8_t *buffer;
    uint32_t head;
    uint32_t tail;
    uint32_t size;
    uint8_t is_full;
    mutex_t mutex;

    uint8_t (*add)(struct buffer*, void*, uint32_t);
    uint32_t (*get)(struct buffer*, void*, uint32_t);
    uint32_t (*get_until)(struct buffer*, void*, uint32_t, uint8_t);
    uint32_t (*get_free_space)(struct buffer*);
    void (*clear)(struct buffer*);
    void (*free)(struct buffer*);
} buffer_t;

buffer_t* create_buffer();

#endif
