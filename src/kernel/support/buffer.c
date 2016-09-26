#include <stddef.h>
#include "buffer.h"
#include "liballoc.h"
#include "string.h"
#include "debug.h"

uint8_t buffer_add(buffer_t *buffer, void *payload, uint32_t size)
{
    mutex_lock(&buffer->mutex);

    if (size > buffer->size)
    {
        mutex_release(&buffer->mutex);
        return BUFFER_FULL;
    }

    // cant use get_free_space because of mutex in mutex
    uint32_t space = (buffer->tail - buffer->head + buffer->size) % buffer->size;
    if (space == 0)
    {
        space = buffer->size;
    }

    if (space < size)
    {
        mutex_release(&buffer->mutex);
        return BUFFER_FULL;
    }

    while(size--)
    {
        buffer->buffer[buffer->head] = *(uint8_t*)payload++;
        buffer->head = (buffer->head + 1) % buffer->size;
    }

    if (buffer->tail == buffer->head)
    {
        buffer->is_full = 1;
    }

    mutex_release(&buffer->mutex);
    return BUFFER_OK;
}

uint32_t buffer_get(buffer_t *buffer, void *dest, uint32_t size)
{
    mutex_lock(&buffer->mutex);

    if (buffer->tail == buffer->head)
    {
        mutex_release(&buffer->mutex);
        return 0;
    }

    uint32_t copied = 0;
    uint8_t *p = (uint8_t*)dest;
    while(buffer->head != buffer->tail && copied < size)
    {
        *p++ = buffer->buffer[buffer->tail];
        buffer->tail = (buffer->tail + 1) % buffer->size;
        copied++;
    }

    if (buffer->tail == buffer->head)
    {
        buffer->is_full = 0;
    }

    mutex_release(&buffer->mutex);
    return copied;
}

uint32_t buffer_get_free_space(buffer_t *buffer)
{
    mutex_lock(&buffer->mutex);

    uint32_t space = (buffer->tail - buffer->head + buffer->size) % buffer->size;
    if (space == 0)
    {
        space = buffer->size;
    }

    mutex_release(&buffer->mutex);
    return space;
}

void buffer_free(buffer_t *buffer)
{
    kfree(buffer->buffer);
    kfree(buffer);
}

buffer_t* create_buffer(uint32_t size)
{
    buffer_t *buffer = kmalloc(sizeof(buffer_t));
    memset(buffer, 0, sizeof(buffer_t));

    buffer->buffer = kmalloc(size);
    memset(buffer->buffer, 0, size);

    buffer->size = size;
    buffer->add = &buffer_add;
    buffer->get = &buffer_get;
    buffer->get_free_space = &buffer_get_free_space;
    buffer->free = &buffer_free;

    return buffer;
}
