#ifndef H_MUTEX
#define H_MUTEX

#include <stdint.h>


typedef struct mutex {
    struct sleep_item *queue;
    volatile uint8_t flag;
}  mutex_t;

//#define mutex_lock(m) debug("mutex lock at %s: %i\n", __FILE__, __LINE__); _mutex_lock(m)
//#define mutex_release(m) debug("mutex release at %s: %i\n", __FILE__, __LINE__); _mutex_release(m)

void mutex_lock(mutex_t *mutex);
void mutex_release(mutex_t *mutex);

#endif
