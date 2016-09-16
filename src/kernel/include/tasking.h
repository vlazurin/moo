#ifndef H_MULTITASKING
#define H_MULTITASKING

#include <stdint.h>
#include "list.h"
#include "system.h"

typedef struct registers
{
    uint32_t ebp;
    uint32_t esp;
    uint32_t eip;
} __attribute__((packed)) volatile registers_t;

enum
{
    THREAD_STATE_RUNNING = 1,
    THREAD_STATE_TERMINATED
};

typedef struct process process_t;
typedef struct thread thread_t;

struct thread
{
    list_node_t list;
    uint32_t id;
    registers_t regs;
    process_t *process;
    uint8_t state;
};

struct process
{
    list_node_t list;
    uint32_t id;
    thread_t *threads;
    uint32_t next_thread_id;
    page_directory_t *page_dir;
    void *page_dir_base;
};

void init_tasking();
uint32_t read_eip();
void switch_task();
void create_thread(void *entry_point, void* params);
void force_task_switch();
void create_process(void *entry_point);

#endif
