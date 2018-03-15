#ifndef H_TASK
#define H_TASK

#include <stdint.h>
#include "list.h"
#include "ring.h"
#include "system.h"
#include "irq.h"
#include "vfs.h"
#include "signal.h"
#include "event.h"

#define THREAD_RUNNING 1
#define THREAD_STOPED 2
#define THREAD_SLEEPING 3
#define PROCESS_RUNNING 1
#define PROCESS_STOPED 2
#define PROCESS_DEAD 3

#define PUSH_STACK(stack, value) (stack) -= sizeof(uint32_t); *((uint32_t*)(stack)) = (uint32_t)(value);

#define THREAD_STACK_SIZE 8192

#define WNOHANG 1

struct thread
{
    list_node_t list;
    uint32_t id;
    struct regs regs;
    void* stack_mem;
    struct process *process;
    uint8_t state;
    int volatile ref_count;
    struct regs *user_regs;
};

struct process
{
    list_node_t list;
    int id;
    int parent_id;
    int group_id;
    struct sigaction signals[32];
    ring_t *signals_queue;
    struct thread *threads;
    uint32_t next_thread_id;
    uint32_t state;
    struct event_data *events;
    vfs_file_t *files[MAX_OPENED_FILES];
    page_directory_t *page_dir;
    void *page_dir_base;
    void *brk;
    uint32_t shared_heap;
    struct shm_map *shm_mapping;
    char cur_dir[MAX_PATH_LENGTH];
    int volatile ref_count;
    mutex_t mutex;
};

extern struct process *current_process;
extern struct thread *current_thread;

void init_multitasking();
void switch_task();
void start_thread(void *entry_point, uint32_t arg);
void force_task_switch();
struct process *create_process(void *entry_point, uint32_t arg);
void schedule_process(struct process *p);
uint32_t get_fg_pid();
void set_fg_pid(uint32_t pid);
int get_pid();
int get_p_pid();
int get_gid();
int fork();
void stop_process();
int set_proc_group(int pid, int group_id);
int wait_pid(int pid, int *status, int options);
int execve(char *path, char **argv, char **envp);
struct event_data *get_event();
int send_event(struct event_data *packet);
#endif
