#include <stddef.h>
#include "ring.h"
#include "liballoc.h"
#include "string.h"
#include "log.h"

uint8_t ring_push(ring_t *ring, void *payload)
{
    // if head == tail -> buffer is empty.
    // I don't fill last item in ring, because after head increment it will be equal to tail... thats wrong.
    // So size of buffer is actually size - 1;
    // TODO: fix me
    mutex_lock(&ring->head_mutex);
    uint32_t next = (ring->head + 1) % ring->size;
    if (next == ring->tail)
    {
        mutex_release(&ring->tail_mutex);
        return RING_BUFFER_FULL;
    }

    ring->buffer[ring->head] = (uint32_t)payload;
    ring->head = next;
    mutex_release(&ring->head_mutex);

    return RING_BUFFER_OK;
}

void* ring_pop(ring_t *ring)
{
    mutex_lock(&ring->tail_mutex);
    if (ring->tail == ring->head)
    {
        mutex_release(&ring->tail_mutex);
        return NULL;
    }

    void *payload = (void*)ring->buffer[ring->tail];
    ring->tail = (ring->tail + 1) % ring->size;
    mutex_release(&ring->tail_mutex);
    return payload;
}

/**
 * Risky to use if tail position isn't 0
 */
void* ring_head_pop(ring_t *ring)
{
    mutex_lock(&ring->head_mutex);
    if (ring->tail == ring->head)
    {
        mutex_release(&ring->head_mutex);
        return NULL;
    }

    void *payload = (void*)ring->buffer[ring->head - 1];
    ring->head = (ring->head - 1) % ring->size;
    mutex_release(&ring->head_mutex);
    return payload;
}

void ring_free(ring_t *ring)
{
    void *ptr = ring->pop(ring);
    while(ptr != 0)
    {
        kfree(ptr);
    }
    kfree(ring->buffer);
    kfree(ring);
}

ring_t* create_ring(uint32_t size)
{
    ring_t *ring = kmalloc(sizeof(ring_t));
    memset(ring, 0, sizeof(ring_t));

    ring->buffer = kmalloc(size * sizeof(uint32_t*));
    memset(ring->buffer, 0, size * sizeof(uint32_t*));

    ring->size = size;
    ring->push = &ring_push;
    ring->pop = &ring_pop;
    ring->head_pop = &ring_head_pop;
    ring->free = &ring_free;

    return ring;
}
