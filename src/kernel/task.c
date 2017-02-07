#include "task.h"
#include "errno.h"
#include "liballoc.h"
#include "timer.h"
#include "log.h"
#include "string.h"
#include "mutex.h"
#include "pit.h"
#include <stdbool.h>
#include "irq.h"
#include "mm.h"

extern void perform_task_switch(uint32_t eip, uint32_t ebp, uint32_t esp);
extern uint32_t read_eip();

uint8_t volatile task_switch_required = 0;
static struct process *processes;
static volatile mutex_t global_mutex;
struct process *current_process = 0;
static struct process *fg_process = 0;
 struct thread *current_thread = 0;
static uint32_t next_pid = 0;
void set_kernel_stack(uintptr_t stack);
extern page_directory_t *page_directory;

static void terminate_thread()
{
    assert((uint32_t)current_thread >= KERNEL_SPACE_ADDR);
    cli();
    current_thread->state = THREAD_TERMINATED;
    sti();
    log(KERN_INFO, "(PID %i) thread %i terminated\n", current_process->id, current_thread->id);
    force_task_switch();
}

static void thread_header(void *entry_point, uint32_t arg)
{
    assert((uintptr_t)entry_point >= KERNEL_SPACE_ADDR);
    void (*func)(uint32_t) = entry_point;
    func(arg);
    terminate_thread();
    assert(false && "thread isn't terminated, system is unstable.");
    while(true);
}

static struct thread *create_thread()
{
    struct thread *thread = kmalloc(sizeof(struct thread));
    memset((void*)thread, 0, sizeof(struct thread));
    thread->state = THREAD_RUNNING;
    thread->regs.eip = (uint32_t)&thread_header;
    thread->stack_mem = kmalloc(THREAD_STACK_SIZE);
    // TODO: should stack be aligned on 16 bytes boundary?
    thread->regs.ebp = (uintptr_t)thread->stack_mem + THREAD_STACK_SIZE;
    thread->regs.esp = thread->regs.ebp;
    return thread;
}

void stop_process()
{
    cli();
    struct thread *iterator = current_process->threads;
    while(iterator != 0) {
        iterator->state = THREAD_TERMINATED;
        iterator = (struct thread*)iterator->list.next;
    }
    current_process->state = PROCESS_TERMINATED;
    sti();
    force_task_switch();
}

void start_thread(void *entry_point, uint32_t arg)
{
    assert((uintptr_t)entry_point >= KERNEL_SPACE_ADDR);
    assert(current_process != NULL);
    // failsafe, probably it never will be false :)
    assert(current_process->next_thread_id < 0xFFFFFF00);

    struct thread *thread = create_thread();
    PUSH_STACK(thread->regs.esp, arg);
    PUSH_STACK(thread->regs.esp, entry_point);
     // thread_header must kill thread, but it's good to have return address for failsafe
    PUSH_STACK(thread->regs.esp, &terminate_thread);

    thread->process = current_process;
    thread->id = __sync_fetch_and_add(&current_process->next_thread_id, 1);

    cli();
    add_to_list(current_process->threads, thread);
    sti();
}

struct process *create_process(void *entry_point, uint32_t arg)
{
    struct process *process = kmalloc(sizeof(struct process));
    memset((void*)process, 0, sizeof(struct process));

    // failsafe, probably it never will be false :)
    assert(next_pid < 0xFFFFFF00);

    process->signals_queue = create_ring(100);

    process->id = __sync_fetch_and_add(&next_pid, 1);
    process->page_dir_base = kmalloc(sizeof(page_directory_t) + 0x1000);
    process->page_dir = (page_directory_t*)PAGE_ALIGN((uint32_t)process->page_dir_base);
    strcpy(process->cur_dir, DEFAULT_DIR);

    for(uint32_t i = 0; i < 0x400; i++) {
        if (i < KERNEL_SPACE_START_PAGE_DIR) {
            process->page_dir->directory[i] = 2;
        } else {
            process->page_dir->directory[i] = page_directory->directory[i];
            process->page_dir->pages[i] = page_directory->pages[i];
        }
    }

    struct thread *thread = create_thread();
    thread->id = __sync_fetch_and_add(&process->next_thread_id, 1);
    PUSH_STACK(thread->regs.esp, arg);
    PUSH_STACK(thread->regs.esp, entry_point);
     // thread_header must terminate thread, but it's good to have return address for failsafe
    PUSH_STACK(thread->regs.esp, &terminate_thread);

    thread->process = process;
    process->threads = thread;

    return process;
}

void schedule_process(struct process *p)
{
    assert((uint32_t)p >= KERNEL_SPACE_ADDR && (uint32_t)p->threads >= KERNEL_SPACE_ADDR);
    cli();
    p->state = PROCESS_RUNNING;
    add_to_list(processes, p);
    sti();
}

uint32_t get_pid()
{
    assert((uint32_t)current_process >= KERNEL_SPACE_ADDR);
    return current_process->id;
}

void set_fg_pid(uint32_t pid)
{
    assert((uint32_t)processes >= KERNEL_SPACE_ADDR);
    cli();
    struct process *iterator = processes;
    while(iterator != 0) {
        if (iterator->id == pid) {
            log(KERN_INFO, "foreground PID %i\n", iterator->id);
            fg_process = iterator;
            break;
        }
        iterator = (struct process*)iterator->list.next;
    }
    sti();
}

uint32_t get_fg_pid()
{
    assert((uint32_t)fg_process >= KERNEL_SPACE_ADDR);
    return fg_process->id;
}

extern void return_to_userspace();
int fork()
{
    struct process *p = create_process(0, 0);
    p->parent_id = get_pid();
    strcpy(p->cur_dir, current_process->cur_dir);

    for(uint32_t i = 0; i < MAX_OPENED_FILES; i++) {
        if (p->files[i] != NULL) {
            p->files[i] = kmalloc(sizeof(vfs_file_t));
            memcpy(p->files[i], current_process->files[i], sizeof(vfs_file_t));
            p->files[i]->pid = p->id;
        }
    }

    void *buffer_chunk = kmalloc(0x2000);
    void *buffer = (void*)PAGE_ALIGN((uint32_t)buffer_chunk);
    uint32_t buffer_phys_back = get_physical_address((uint32_t)buffer);

    for(uint32_t i = 0; i < KERNEL_SPACE_START_PAGE_DIR; i++) {
        if (page_directory->directory[i] == 2) {
            continue;
        }

        p->page_dir->page_chunks[i] = kmalloc(0x1000 + 0x1000);
        p->page_dir->pages[i] = (uint32_t*)PAGE_ALIGN((uint32_t)p->page_dir->page_chunks[i]);
        p->page_dir->directory[i] = get_physical_address((uint32_t)p->page_dir->pages[i]) | 7;
        for(uint32_t y = 0; y < 1024; y++) {
            if ((page_directory->pages[i][y] & 7) == 7) {
                uint32_t phys = alloc_physical_page();
                map_virtual_to_physical((uint32_t)buffer, phys);
                memcpy(buffer, (void*)((i * 0x400 + y) * 0x1000), 0x1000);
                p->page_dir->pages[i][y] = phys | 7;
            } else {
                p->page_dir->pages[i][y] = 2;
            }
        }
    }
    map_virtual_to_physical((uint32_t)buffer, buffer_phys_back);
    kfree(buffer_chunk);

    memcpy(p->threads[0].stack_mem + KERNEL_STACK_SIZE - sizeof(struct regs), current_process->user_regs, sizeof(struct regs));
    p->threads[0].regs.esp = (uint32_t)p->threads[0].stack_mem + KERNEL_STACK_SIZE - sizeof(struct regs);
    p->threads[0].regs.ebp = p->threads[0].regs.esp;
    p->threads[0].regs.eip = (uint32_t)&return_to_userspace;
    struct regs *new = (struct regs*)p->threads[0].regs.esp;
    new->eax = 0;
    int child_pid = p->id;
    cli();
    add_to_list(processes, p);
    sti();
    return child_pid;
}

int wait_pid(int pid, int *status, int options)
{
    while(1) {
        if (pid == -1) {
            cli();
            struct process *iterator = processes;
            while(iterator != NULL) {
                if (iterator->parent_id == current_process->id && iterator->state == PROCESS_TERMINATED) {
                    delete_from_list((void*)&processes, iterator);
                    break;
                }
                iterator = (struct process*)iterator->list.next;
            }
            sti();

            if (iterator == NULL) {
                if (options & WNOHANG) {
                    return -ECHILD;
                }
                continue;
            } else {
                //TODO: FREE PROCESS MEMORY
                *status = 0x0;
                return iterator->id;
            }
        }
    }
}

struct process *proc_by_id(int pid)
{
    cli();
    struct process *iterator = processes;
    while(iterator != NULL) {
        if (iterator->id == pid) {
            sti();
            return iterator;
        }
        iterator = (struct process*)iterator->list.next;
    }
    sti();
    return NULL;
}

void init_multitasking()
{
    struct process *process = kmalloc(sizeof(struct process));
    memset((void*)process, 0, sizeof(struct process));
    process->signals_queue = create_ring(100);
    process->state = PROCESS_RUNNING;
    process->next_thread_id = 1;
    process->id = __sync_fetch_and_add(&next_pid, 1);
    process->page_dir_base = (void*)PAGE_DIRECTORY_VIRTUAL;
    process->page_dir = (page_directory_t*)PAGE_DIRECTORY_VIRTUAL;
    strcpy(process->cur_dir, DEFAULT_DIR);

    struct thread *thread = kmalloc(sizeof(struct thread));
    memset((void*)thread, 0, sizeof(struct thread));
    thread->id = 0;
    thread->state = THREAD_RUNNING;
    thread->process = process;
    set_kernel_stack((uint32_t)thread->stack_mem + KERNEL_STACK_SIZE);

    process->threads = thread;

    current_thread = thread;
    current_process = process;

    processes = process;
}

void force_task_switch()
{
    task_switch_required = 1;
    switch_task();
}

void switch_task()
{
    if (task_switch_required == false || current_process == NULL) {
        return;
    }

    // switch_task can be called not only from hardware interrupt handler, so better to disable interrupts
    cli();
    struct thread *th = (struct thread*)current_thread->list.next;
    struct process *ps = current_process;
    // TODO: save fpu state
    while(true) {
        if (th == NULL || ps->state == PROCESS_TERMINATED) {
            ps = (struct process*)current_process->list.next;
            if (ps == 0) {
                ps = processes;
            }
            th = ps->threads;
        }

        if (th->state == THREAD_TERMINATED) {
            th = (struct thread*)th->list.next;
            continue;
        }
        break;
    };

    uint32_t esp;
    uint32_t ebp;
    uint32_t eip;
	asm volatile ("mov %%esp, %0" : "=r" (esp));
	asm volatile ("mov %%ebp, %0" : "=r" (ebp));
	eip = read_eip();
    if (eip == 0x12345) {
        return;
    }
    current_thread->regs.esp = esp;
    current_thread->regs.ebp = ebp;
    current_thread->regs.eip = eip;

    if (ps != current_process) {
        page_directory = ps->page_dir;
        asm("movl %0, %%eax" :: "r"(get_physical_address((uint32_t)page_directory)));
        asm("mov %eax, %cr3");
    }

    current_process = ps;
    current_thread = th;
    set_kernel_stack((uint32_t)th->stack_mem + KERNEL_STACK_SIZE);

    perform_task_switch(current_thread->regs.eip, current_thread->regs.ebp, current_thread->regs.esp);
}
