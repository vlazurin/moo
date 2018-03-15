#include "event.h"
#include "stddef.h"
#include <limits.h>

static int event_stream = -1;
static void* handlers[USHRT_MAX] = {0};
static uint8_t handlers_mutex = 0;

void lock(uint8_t *lock)
{
    while (__sync_lock_test_and_set(lock, 1)){}
}

void unlock(uint8_t *lock)
{
     __sync_lock_release(lock);
}

int init_event_loop()
{
    if (event_stream != -1) {
        return -EEXIST;
    }

    event_stream = open("/dev/event", O_RDWR);
    if (event_stream == -1) {
        return -1;
    }

    return 0;
}

int add_event_handler()
{

}

int remove_event_handler()
{

}

void process_event_loop()
{
    int done = 0;
    struct event_data *packet = malloc(MAX_EVENT_PACKET_SIZE);
    void (*handler)(struct event_data*);

    while (true) {
        done = read(event_stream, packet, MAX_EVENT_PACKET_SIZE);

        if (done == 0) { // events read is blocking syscall, but better to have this check. Mb i will introduce timeout later...
            continue;
        }

        // lock isn't needed. In worst case we may call wrong handler. But it may occur anyway because of queue.
        handler = handlers[packet->event_type];
        if (handler != NULL) {
            handler(packet);
            continue;
        }

        if (done == -1) {
            printf("ops, event loop can't read events, something is really unstable, errno = %i\n", errno);
            break;
        }
    }

    free(packet);
    printf("end of process_event_loop method... how we get here?");
    return -1;
}
