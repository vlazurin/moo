#include "timer.h"
#include "liballoc.h"
#include "pit.h"
#include "task.h"
#include <stdbool.h>
#include "mutex.h"

timer_t *timers = 0;
mutex_t timers_mutex = {0};

void process_timers()
{
    while(true)
    {
        mutex_lock(&timers_mutex);

        timer_t *iterator = timers;
        while(iterator != NULL)
        {
            if (get_pit_ticks() >= iterator->finish_ticks)
            {
                timer_t *tmp = iterator;
                iterator = (timer_t*)iterator->list.next;
                delete_from_list((void*)&timers, tmp);
                // after this line timer can not exist(deleted by waiting thread)
                __sync_val_compare_and_swap(&tmp->busy, 1, 0);
            }
        }

        mutex_release(&timers_mutex);
        force_task_switch();
    }
}

void init_timer()
{
    start_thread(process_timers, 0);
}

void set_timer(timer_t *timer)
{
    timer->finish_ticks = get_pit_ticks() + timer->interval;
    timer->busy = 1;
    mutex_lock(&timers_mutex);
    add_to_list(timers, timer);
    mutex_release(&timers_mutex);
}

void sleep(uint32_t delay)
{
    uint32_t finish_ticks = get_pit_ticks() + delay;
    while(finish_ticks > get_pit_ticks())
    {
        force_task_switch();
    }
}
