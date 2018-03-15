#include "vfs.h"
#include "liballoc.h"
#include "string.h"
#include "task.h"
#include "errno.h"
#include "log.h"
#include <stddef.h>

static int write(vfs_file_t *file, void *buf, uint32_t size, uint32_t *offset)
{
    if (size > MAX_EVENT_PACKET_SIZE) {
        return -EFBIG;
    }

    struct event_data *packet = kmalloc(size);
    memcpy(packet, buf, size);
    int errno = send_event(packet);
    if (errno != 0) {
        kfree(packet);
        return errno;
    }
    return size;
}

static int read(vfs_file_t *file, void *buf, uint32_t size, uint32_t *offset)
{
    struct event_data *packet = NULL;
    // always perform blocking read. TODO: Must be fixed in future versions.
    while(packet == NULL) {
        packet = get_event();
    }

    if (packet->size > size) {
        kfree(packet);
        return -ENOMEM;
    }

    memcpy(buf, packet, packet->size);
    int done = packet->size;
    kfree(packet);
    return done;
}

static vfs_file_operations_t file_ops = {
    .open = 0,
    .close = 0,
    .write = &write,
    .read = &read
};

void init_events()
{
    create_vfs_node("/dev/event", S_IFCHR, &file_ops, NULL, NULL);
}
