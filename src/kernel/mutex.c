#include "./include/mutex.h"
//#include "./include/multitasking.h"

void mutex_lock(mutex_t *mutex)
{
    while (__sync_lock_test_and_set(mutex, 1))
    {
        //request_task_switch();
    }
}

void mutex_release(mutex_t *mutex)
{
     __sync_lock_release(mutex);
}
