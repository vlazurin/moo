#include "vfs.h"
#include "debug.h"

// TODO: must be uint64_t but debug() doesn't support uint64_t
uint32_t nulled_count = 0;

static uint32_t write(vfs_file_t *file, void *buf, uint32_t size)
{
    __sync_add_and_fetch(&nulled_count, size);
    debug("[null] received %i bytes\n", nulled_count);
    return size;
}

vfs_file_operations_t null_file_ops = {
    .open = 0,
    .close = 0,
    .write = &write,
    .read = 0
};

void init_null()
{
    create_vfs_device("/dev/null", &null_file_ops, 0);
}
