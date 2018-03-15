#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "event.h"

#define assert(e) ((e) ? (void)0 : printf(KERN_INFO, "assert failed: %s:%s():%i. Expression: %s\n", __FILE__, __func__, __LINE__, #e))

void first_handler(struct event_data *packet)
{

}

void second_handler(struct event_data *packet)
{

}

int main()
{
    int err = init_event_loop();
    assert(err == 0);
    err = init_event_loop();
    assert(err == -EEXIST);


    return 0;
}
