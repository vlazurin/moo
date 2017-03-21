#include "vfs.h"
#include "mutex.h"
#include "rand.h"
#include "log.h"

mutex_t mutex = {0};
pcg32_random_t rand = { 0xF15B48A0C371D4EBULL, 0xDB4592AF6B9C28F2ULL };

uint32_t get_random()
{
    mutex_lock(&mutex);
    uint32_t result = pcg32_random_r(&rand);
    mutex_release(&mutex);

    return result;
}

int urandom_read(vfs_file_t *file, void *buf, uint32_t size, uint32_t *offset)
{
    uint32_t done = 0;
    uint8_t shift = 0;
    uint32_t random = get_random();
    while(done < size)
    {
        *(uint8_t*)buf = ((random >> shift) & 0xFF);
        buf++;
        done++;
        shift += 8;
        if (shift > 32)
        {
            shift = 0;
            random = get_random();
        }
    }

    return done;
}

vfs_file_operations_t urand_file_ops = {
    .open = 0,
    .close = 0,
    .write = 0,
    .read = &urandom_read
};

void init_urandom()
{
    create_vfs_node("/dev/urandom", S_IFCHR, &urand_file_ops, 0, 0);
}
