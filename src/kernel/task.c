#include "task.h"
#include "liballoc.h"
#include "timer.h"
#include "debug.h"
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
static struct thread *current_thread = 0;
static uint32_t next_pid = 0;

extern page_directory_t *page_directory;

static void terminate_thread()
{
    assert((uint32_t)current_thread >= KERNEL_SPACE_ADDR);
    cli();
    current_thread->state = THREAD_TERMINATED;
    sti();
    debug("[kernel] (PID %i) thread %i state is TERMINATED\n", current_process->id, current_thread->id);
    force_task_switch();
}

static void thread_header(void *entry_point, uint32_t arg)
{
    assert((uintptr_t)entry_point >= KERNEL_SPACE_ADDR);
    void (*func)(uint32_t) = entry_point;
    func(arg);
    terminate_thread();
    assert(false, "Thread isn't terminated, system is unstable");
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

    // TODO: stupid hack
    process->page_dir->directory[0] = page_directory->directory[0];

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
    while(iterator != 0)
    {
        if (iterator->id == pid)
        {
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

int fork(struct regs *r)
{
    return 0;
    /*struct process *p = create_process(0, 0);
    p->page_dir = current_process->page_dir;

    uint32_t stack_size = (uintptr_t)current_thread->stack_mem - (uint32_t)r;
    r->eax = 0;
    memcpy((void*)p->threads[0].stack_mem - stack_size, (uint8_t*)r, stack_size);
    p->threads[0].regs.ebp = (uintptr_t)p->threads[0].stack_mem;
    p->threads[0].regs.esp = (uintptr_t)p->threads[0].stack_mem - stack_size;
    p->threads[0].regs.eip = (uint32_t)&return_to_userspace;
    //TODO: change eax, because of fork return value
    debug("eax %h, ebx %h, eip %h\n", r->eax, r->ebx, r->eip);
    cli();
    add_to_list(processes, p);
    sti();
    while(1){}
    return get_pid();*/
}

void init_multitasking()
{
    struct process *process = kmalloc(sizeof(struct process));
    memset((void*)process, 0, sizeof(struct process));
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
        if (th == NULL) {
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

    perform_task_switch(current_thread->regs.eip, current_thread->regs.ebp, current_thread->regs.esp);
}
