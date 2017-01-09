#ifndef H_MULTITASKING
#define H_MULTITASKING

#include <stdint.h>
#include "list.h"
#include "system.h"
#include "irq.h"
#include "vfs.h"

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
    uint32_t base_ebp;
    process_t *process;
    uint8_t state;
};

struct process
{
    list_node_t list;
    uint32_t id;
    thread_t *threads;
    uint32_t next_thread_id;
    vfs_file_t *files[MAX_OPENED_FILES];
    page_directory_t *page_dir;
    void *page_dir_base;
    void *brk;
};

void init_tasking();
uint32_t read_eip();
void switch_task();
void create_thread(void *entry_point, void* params);
void force_task_switch();
process_t *create_process(void *entry_point, void* params);
void schedule_process(process_t *p);
uint32_t get_fg_process_pid();
void set_fg_process(uint32_t pid);
uint32_t get_pid();
process_t *get_curent_proccess();
int fork(struct regs *r);

#endif
