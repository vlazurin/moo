#include "vfs.h"
#include "mutex.h"
#include "port.h"
#include "log.h"
#include "buffer.h"
#include "irq.h"

extern unsigned long long l_allocated;
extern unsigned long long l_inuse;

static int mem_read(vfs_file_t *file, void *buf, uint32_t size, uint32_t *offset)
{
    int bytes = sizeof(long long) * 2;
    if (size < bytes) {
        return 0;
    }
    long long* ptr = buf;
    *ptr++ = l_allocated;
    *ptr = l_inuse;
    return bytes;
}

static vfs_file_operations_t mem_file_ops = {
    .open = 0,
    .close = 0,
    .write = 0,
    .read = &mem_read
};

void init_mem()
{
    create_vfs_node("/dev/stat_mem", S_IFCHR, &mem_file_ops, (void*)0, 0);
}
