#include "./include/mutex.h"
#include "task.h"
#include <stddef.h>
#include "liballoc.h"

void mutex_lock(mutex_t *mutex)
{
    while (__sync_lock_test_and_set(&mutex->flag, 1))
    {
        force_task_switch();
    }
}

void mutex_release(mutex_t *mutex)
{
     __sync_lock_release(&mutex->flag);
}
