#include <stdint.h>
#include <stdbool.h>
#include "errno.h"
#include "task.h"
#include "string.h"
#include "log.h"
#include "liballoc.h"
#include "mm.h"
#include "shm.h"

static mutex_t shm_mutex = {0};
static struct shm_segment *segments = NULL;

static void test_shm();

// function isn't thread safe
static struct shm_segment *get_segment(const char *name)
{
    FOR_EACH(item, segments, struct shm_segment) {
        if (strcmp(item->name, name) == 0) {
            return item;
        }
    }
    return NULL;
}

static void free_segment(struct shm_segment *segment)
{
    assert(segment->ref_count > 0 && "ref counting for shared memory is really broken");

    if (ref_dec(&segment->ref_count) > 0 || segment->persistent == true) {
        return;
    }

    mutex_lock(&shm_mutex);
    delete_from_list((void*)&segments, segment);
    mutex_release(&shm_mutex);

    for(int i = 0; i < segment->pages_count; i++) {
        free_physical_page(segment->pages[i]);
    }
    kfree(segment->pages);
    debug("shared memory \"%s\" is deleted\n", segment->name);
    kfree(segment);
}

int shm_unmap(const char *name)
{
    struct shm_map *mapping = NULL;

    mutex_lock(&current_process->mutex);
    FOR_EACH(item, current_process->shm_mapping, struct shm_map) {
        if (strcmp(item->segment->name, name) == 0) {
            mapping = item;
        }
    }

    if (mapping == NULL) {
        mutex_release(&current_process->mutex);
        return -EINVAL;
    }

    uint32_t addr = mapping->addr;
    for (int i = 0; i < mapping->segment->pages_count; i++) {
        unmap_page(addr);
        addr += 0x1000;
    }

    debug("shared memory \"%s\" is unmapped\n", mapping->segment->name);
    free_segment(mapping->segment);
    delete_from_list((void*)&current_process->shm_mapping, mapping);
    kfree(mapping);
    mutex_release(&current_process->mutex);

    return 0;
}

uint32_t shm_get_addr(const char *name)
{
    uint32_t addr = 0;

    mutex_lock(&current_process->mutex);
    FOR_EACH(item, current_process->shm_mapping, struct shm_map) {
        if (strcmp(item->segment->name, name) == 0) {
            addr = item->addr;
            break;
        }
    }
    mutex_release(&current_process->mutex);

    return addr;
}

static int do_mapping(struct shm_segment *seg)
{
    struct shm_map *insert_after = NULL;
    uint8_t gap_found = false;
    int errno = 0;
    uintptr_t addr = USERSPACE_SHARED_MEM_TOP - seg->size;

    mutex_lock(&current_process->mutex);
    // Very stupid algorithm, probably some good memory allocator must be used.
    // I will keep it for future improvements :)
    //
    // List should be ordered by address in desc order
    FOR_EACH(item, current_process->shm_mapping, struct shm_map) {
        if (item->list.next != NULL) {
            struct shm_map *next = item->list.next;
            if (item->addr - (next->addr + next->segment->size) >= seg->size) {
                insert_after = item;
                gap_found = true;
                addr = item->addr - seg->size;
                break;
            }
        } else if (item->addr - seg->size >= USERSPACE_SHARED_MEM) {
            insert_after = item;
            gap_found = true;
            addr = item->addr - seg->size;
            break;
        }
    }

    if ((gap_found == false && current_process->shm_mapping != NULL) || addr < USERSPACE_SHARED_MEM) {
        errno = -ENOMEM;
        goto mutex_cleanup;
    }

    struct shm_map *map = kmalloc(sizeof(struct shm_map));
    memset(map, 0, sizeof(struct shm_map));
    map->segment = seg;
    map->addr = addr;

    for (int i = 0; i < seg->pages_count; i++) {
        map_virtual_to_physical(addr, seg->pages[i], 0);
        addr += 0x1000;
    }

    if (current_process->shm_mapping == NULL) {
        add_to_list(current_process->shm_mapping, map);
    } else if (gap_found == true) {
        insert_in_list(map, insert_after);
    }

mutex_cleanup:
    mutex_release(&current_process->mutex);
    if (errno) {
        log(KERN_ERR, "do_mapping (\"%s\"), errno %i\n", seg->name, errno);
        kfree(map);
    }
    return errno;
}

int shm_map(const char *name)
{
    int errno = 0;
    struct shm_segment *seg = NULL;

    mutex_lock(&shm_mutex);
    seg = get_segment(name);
    if (seg == NULL) {
        mutex_release(&shm_mutex);
        return -EINVAL;
    }
    ref_inc(&seg->ref_count);
    mutex_release(&shm_mutex);

    errno = do_mapping(seg);
    if (errno != 0) {
        free_segment(seg);
    }

    return errno;
}

static struct shm_segment *create_segment(const char *name, size_t size, uintptr_t *pages)
{
    size_t name_length = strlen(name);
    struct shm_segment *seg = kmalloc(sizeof(struct shm_segment));
    memset(seg, 0, sizeof(struct shm_segment));
    memcpy(seg->name, name, name_length);
    seg->size = size;
    seg->pages_count = size / 0x1000;

    int count = seg->pages_count;
    size_t bytes = count * sizeof(uintptr_t);
    seg->pages = kmalloc(bytes);
    if (pages == NULL) {
        while (count >= 0) {
            seg->pages[count] = alloc_physical_page();
            count--;
        }
    } else {
        memcpy(seg->pages, pages, bytes);
    }

    return seg;
}

int shm_alloc(const char *name, uint32_t size, uintptr_t *pages, uint8_t persistent)
{
    assert(strpos(name, '/') == -1 && "dangerous to have '/' in shared memory segment name, if segment may be accessed via VFS");

    int errno = 0;
    size_t name_length = strlen(name);
    struct shm_segment *seg = NULL;

    if (size == 0) {
        errno = -EINVAL;
        goto cleanup;
    }

    size = PAGE_ALIGN(size);
    if (name_length == 0 || name_length > SHM_SEGMENT_NAME_LENGTH || size > USERSPACE_SHARED_MEM_SIZE) {
        errno = -EINVAL;
        goto cleanup;
    }

    mutex_lock(&shm_mutex);
    if (get_segment(name) != NULL) {
        errno = -EEXIST;
        goto mutex_cleanup;
    }
    seg = create_segment(name, size, pages);
    seg->persistent = persistent;

    if (seg->persistent == false) {
        errno = do_mapping(seg);
        if (errno != 0) {
            goto mutex_cleanup;
        }
        ref_inc(&seg->ref_count);
    }
    add_to_list(segments, seg);

mutex_cleanup:
    mutex_release(&shm_mutex);
cleanup:
    if (errno) {
        if (seg != NULL) {
            if (seg->pages != NULL && pages == NULL) {
                int limit = size / 0x1000;
                for(int i = 0; i < limit; i++) {
                    free_physical_page(seg->pages[i]);
                }
                kfree(seg->pages);
            }
            kfree(seg);
        }
    }
    log(KERN_INFO, "shm_alloc (\"%s\", %d), errno %i\n", name, size, errno);
    return errno;
}

void init_shm()
{
    #ifdef RUN_TESTS
    test_shm();
    #endif
}

static void test_shm()
{
    return;
    // segment size should be not 0
    int errno = shm_alloc("test", 0, NULL, false);
    assert(errno == -EINVAL);
    assert(MUTEX_IS_SET(shm_mutex) == 0);
    assert(current_process->shm_mapping == NULL);

    // segment size must be less than USERSPACE_SHARED_MEM_SIZE
    errno = shm_alloc("test", USERSPACE_SHARED_MEM_SIZE + 1, NULL, false);
    assert(errno == -EINVAL);
    assert(MUTEX_IS_SET(shm_mutex) == 0);
    assert(current_process->shm_mapping == NULL);

    // segment name can't be empty
    errno = shm_alloc("", 0, NULL, false);
    assert(errno == -EINVAL);
    assert(MUTEX_IS_SET(shm_mutex) == 0);
    assert(current_process->shm_mapping == NULL);

    // segment name length must be less than SHM_SEGMENT_NAME_LENGTH
    char long_name[SHM_SEGMENT_NAME_LENGTH + 2];
    memset(long_name, 'a', SHM_SEGMENT_NAME_LENGTH + 2);
    long_name[SHM_SEGMENT_NAME_LENGTH + 1] = '\0';
    errno = shm_alloc(long_name, 100, NULL, true);
    assert(errno == -EINVAL);
    assert(MUTEX_IS_SET(shm_mutex) == 0);
    assert(current_process->shm_mapping == NULL);

    // must be successful && size must be page aligned
    errno = shm_alloc("first", 100, NULL, false);
    assert(errno == 0);
    assert(MUTEX_IS_SET(shm_mutex) == 0);
    assert(segments != NULL);
    assert(strcmp(segments->name, "first") == 0);
    assert(segments->size == 0x1000);
    assert(segments->pages != NULL);
    assert(segments->ref_count == 1);
    assert(current_process->shm_mapping != NULL);
    assert(current_process->shm_mapping->addr == USERSPACE_SHARED_MEM_TOP - 0x1000);
    assert(current_process->shm_mapping->segment == segments);

    // must be successful
    errno = shm_alloc("second", 0x2000, NULL, false);
    assert(errno == 0);
    assert(MUTEX_IS_SET(shm_mutex) == 0);
    assert(segments != NULL);
    assert(strcmp(segments->name, "second") == 0);
    assert(segments != NULL);
    assert(segments->size == 0x2000);
    assert(segments->ref_count == 1);
    assert(segments->pages != NULL);
    struct shm_segment *first = segments->list.next;
    assert(strcmp(first->name, "first") == 0);
    struct shm_map *second_mapping = current_process->shm_mapping->list.next;
    assert(second_mapping != NULL);
    assert(second_mapping->addr == USERSPACE_SHARED_MEM_TOP - 0x3000);
    assert(second_mapping->segment == segments);

    // must fail because segment with same name exists
    errno = shm_alloc("second", 500, NULL, false);
    assert(errno == -EEXIST);

    errno = shm_alloc("third", 500, NULL, true);
    assert(errno == 0);
    uintptr_t addr = shm_get_addr("third");
    assert(addr == 0);
    errno = shm_map("third");
    assert(errno == 0);
    addr = shm_get_addr("third");
    assert(addr > 0);

    // segment must be correctly unmapped and deleted
    errno = shm_unmap("second");
    assert(errno == 0);
    struct shm_map *third_mapping = current_process->shm_mapping->list.next;
    assert_kspace(third_mapping);
    assert(strcmp(third_mapping->segment->name, "third") == 0);

    // segment must be allocated between "first" and "third"
    uintptr_t pages[2];
    pages[0] = alloc_physical_page();
    pages[1] = alloc_physical_page();
    errno = shm_alloc("second", 0x2000, pages, false);
    assert(errno == 0);
    second_mapping = current_process->shm_mapping->list.next;
    assert(second_mapping->segment->pages[0] == pages[0]);
    assert(second_mapping->segment->pages[1] == pages[1]);
    assert(strcmp(second_mapping->segment->name, "second") == 0);
    assert(second_mapping->addr == USERSPACE_SHARED_MEM_TOP - 0x3000);

    errno = shm_unmap("second");
    assert(errno == 0);
    free_physical_page(pages[0]);
    free_physical_page(pages[1]);

    addr = shm_get_addr("first");
    assert(addr == USERSPACE_SHARED_MEM_TOP - 0x1000);

    addr = shm_get_addr("second");
    assert(addr == 0);

    addr = shm_get_addr("third");
    assert(addr == USERSPACE_SHARED_MEM_TOP - 0x4000);

    shm_unmap("first");
    segments->persistent = 0;
    shm_unmap("third");
}
