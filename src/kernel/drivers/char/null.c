#include "vfs.h"
#include "log.h"

// TODO: must be uint64_t but debug() doesn't support uint64_t
uint32_t nulled_count = 0;

static int write(vfs_file_t *file, void *buf, uint32_t size, uint32_t *offset)
{
    __sync_add_and_fetch(&nulled_count, size);
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
    create_vfs_node("/dev/null", S_IFCHR, &null_file_ops, 0, 0);
}
