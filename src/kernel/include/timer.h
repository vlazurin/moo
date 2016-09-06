#ifndef H_TIMER
#define H_TIMER

#include <stdint.h>
#include "list.h"

typedef struct timer
{
    list_node_t list;
    uint32_t interval;
    uint32_t finish_ticks;
    uint8_t busy;
} timer_t;

void set_timer(timer_t *timer);
void sleep(uint32_t delay);
void init_timer();

#endif
