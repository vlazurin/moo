#include "timer.h"
#include "liballoc.h"
#include "pit.h"
#include "tasking.h"
#include "list.h"
#include <stdbool.h>
#include "mutex.h"

list_t *timers;

void process_timers()
{
    while(true)
    {
        mutex_lock(&timers->mutex);

        list_node_t *iterator = timers->root;
        timer_t *timer = NULL;
        while(iterator != NULL)
        {
            timer = (timer_t*)iterator->data;
            if (get_pit_ticks() >= timer->finish_ticks)
            {
                list_node_t *for_exclude = iterator;
                iterator = iterator->next;
                timers->delete_node(timers, for_exclude);
                // after this line timer can not exist(deleted by waiting thread)
                __sync_val_compare_and_swap(&timer->busy, 1, 0);
            }
        }

        mutex_release(&timers->mutex);
        force_task_switch();
    }
}

void init_timer()
{
    timers = create_list();
    create_thread(process_timers, NULL);
}

void set_timer(timer_t *timer)
{
    timer->finish_ticks = get_pit_ticks() + timer->interval;
    timer->busy = 1;
    timers->add(timers, timer);
}

void sleep(uint32_t delay)
{
    uint32_t finish_ticks = get_pit_ticks() + delay;
    while(finish_ticks < get_pit_ticks())
    {
        force_task_switch();
    }
}
