#include "tasking.h"
#include "liballoc.h"
#include "timer.h"
#include "debug.h"
#include "string.h"
#include "mutex.h"
#include "pit.h"
#include "irq.h"
#include "mm.h"

uint8_t volatile task_switch_required = 0;
uint8_t tasking_enabled = 0;
process_t *processes;
process_t *current_process = 0;
process_t *fg_process = 0;
thread_t *current_thread = 0;
uint32_t next_pid = 0;

// process 0 page directory
extern page_directory_t *page_directory;
extern void return_to_userspace();

int fork(struct regs *r)
{
    return 0;
    process_t *p = create_process(0, 0);
    p->page_dir = current_process->page_dir;

    uint32_t stack_size = current_thread->base_ebp - (uint32_t)r;
    r->eax = 0;
    memcpy((void*)p->threads[0].base_ebp - stack_size, (uint8_t*)r, stack_size);
    p->threads[0].regs.ebp = p->threads[0].base_ebp;
    p->threads[0].regs.esp = p->threads[0].base_ebp - stack_size;
    p->threads[0].regs.eip = (uint32_t)&return_to_userspace;
    //TODO: change eax, because of fork return value
    debug("eax %h, ebx %h, eip %h\n", r->eax, r->ebx, r->eip);
    cli();
    add_to_list(processes, p);
    sti();
    while(1){}
    return get_pid();
}

uint32_t get_pid()
{
    return current_process->id;
}

process_t *get_curent_proccess()
{
    return current_process;
}

void set_fg_process(uint32_t pid)
{
    cli();
    process_t *iterator = processes;
    while(iterator != 0)
    {
        if (iterator->id == pid)
        {
            fg_process = iterator;
            break;
        }
        iterator = (process_t*)iterator->list.next;
    }
    sti();
}

uint32_t get_fg_process_pid()
{
    uint32_t pid = 0;
    cli();
    if (fg_process != 0)
    {
        pid = fg_process->id;
    }
    sti();
    return pid;
}

void terminate_thread()
{
    cli();
    debug("[kernel] thread %i terminated in process %i\n", current_thread->id, current_thread->process->id);
    current_thread->state = THREAD_STATE_TERMINATED;
    sti();
    force_task_switch();
}

void thread_header(void *entry_point, void *params)
{
    sti();
    void (*func)(void*) = entry_point;
    func(params);
    terminate_thread();
}

void create_thread(void *entry_point, void* params)
{
    thread_t *thread = kmalloc(sizeof(thread_t));
    memset((void*)thread, 0, sizeof(thread_t));
    thread->state = THREAD_STATE_RUNNING;
    thread->regs.eip = (uint32_t)&thread_header;
    thread->base_ebp = (uint32_t)kmalloc(8192) + 8192;
    thread->regs.ebp = thread->base_ebp;
    thread->regs.esp = thread->regs.ebp;
    // stack pointer points on last pushed item (push instruction will preincrement it)
    *((uint32_t*)thread->regs.esp) = (uint32_t)params;
    thread->regs.esp -= 4;
    *((uint32_t*)thread->regs.esp) = (uint32_t)entry_point;
    thread->regs.esp -= 4;
     // thread_header must terminate thread, but it's good to have return address for failsafe
    *((uint32_t*)thread->regs.esp) = (uint32_t)&terminate_thread;

    thread->process = current_thread->process;

    cli();
    thread->id = current_thread->process->next_thread_id++;
    add_to_list(current_thread->process->threads, thread);
    sti();
}

void schedule_process(process_t *p)
{
    cli();
    add_to_list(processes, p);
    sti();
}

process_t *create_process(void *entry_point, void* params)
{
    process_t *process = kmalloc(sizeof(process_t));
    memset((void*)process, 0, sizeof(process_t));

    process->id = __sync_fetch_and_add(&next_pid, 1);
    process->page_dir_base = kmalloc(sizeof(page_directory_t) + 0x1000);
    process->page_dir = (void*)PAGE_ALIGN((uint32_t)process->page_dir_base);

    for(uint32_t i = 0; i < 0x387; i++)
    {
        process->page_dir->directory[i] = 2;
    }

    for(uint32_t i = 0x387; i < 1024; i++)
    {
        process->page_dir->directory[i] = page_directory->directory[i];
        process->page_dir->pages[i] = page_directory->pages[i];
    }

    // stupid hack
    process->page_dir->directory[0] = page_directory->directory[0];

    thread_t *thread = kmalloc(sizeof(thread_t));
    memset(thread, 0, sizeof(thread_t));
    thread->id = process->next_thread_id++;
    thread->state = THREAD_STATE_RUNNING;
    thread->regs.eip = (uint32_t)&thread_header;
    thread->base_ebp = (uint32_t)kmalloc(8192) + 8192;
    thread->regs.ebp = thread->base_ebp;
    thread->regs.esp = thread->regs.ebp;
    // stack pointer points on last pushed item (push instruction will preincrement it)
    *((uint32_t*)thread->regs.esp) = (uint32_t)params;
    thread->regs.esp -= 4;
    *((uint32_t*)thread->regs.esp) = 0;
    thread->regs.esp -= 4;
    *((uint32_t*)thread->regs.esp) = (uint32_t)entry_point;
    thread->regs.esp -= 4;
     // thread_header must terminate thread, but it's good to have return address for failsafe
    *((uint32_t*)thread->regs.esp) = (uint32_t)&terminate_thread;

    thread->process = process;
    process->threads = thread;

    return process;
}

void init_tasking()
{
    process_t *process = kmalloc(sizeof(process_t));
    memset((void*)process, 0, sizeof(process_t));
    process->next_thread_id = 1;
    process->id = __sync_fetch_and_add(&next_pid, 1);
    process->page_dir_base = (void*)PAGE_DIRECTORY_VIRTUAL;
    process->page_dir = (page_directory_t*)PAGE_DIRECTORY_VIRTUAL;

    thread_t *thread = kmalloc(sizeof(thread_t));
    memset((void*)thread, 0, sizeof(thread_t));
    thread->id = 0;
    thread->state = THREAD_STATE_RUNNING;
    thread->process = process;

    process->threads = thread;

    current_thread = thread;
    current_process = process;

    processes = process;

    task_switch_required = 0;
    tasking_enabled = 1;
}

void force_task_switch()
{
    task_switch_required = 1;
    switch_task();
}

void switch_task()
{
    if (tasking_enabled == 0)
    {
        return;
    }

    if (task_switch_required == 0)
    {
        return;
    }

    thread_t *prev;
    thread_t *next;
    cli();
    task_switch_required = 0;
    next = (thread_t*)current_thread->list.next;
    prev = current_thread;
    process_t *next_proc = current_process;
    // TODO: save coprocessor state
    while(1)
    {
        if (next == 0)
        {
            next_proc = (process_t*)current_process->list.next;
            if (next_proc == 0)
            {
                next_proc = processes;
            }
            next = next_proc->threads;
        }

        if (next->state == THREAD_STATE_TERMINATED)
        {
            next = (thread_t*)next->list.next;
            continue;
        }
        break;
    };

    if (next_proc != current_process)
    {
        page_directory = next_proc->page_dir;
        asm("movl %0, %%eax" :: "r"(get_physical_address((uint32_t)next_proc->page_dir)));
        asm("mov %eax, %cr3");
    }

    current_process = next_proc;
    current_thread = next;

    asm("pushal");
    asm("pushf");
    asm("mov %0, %%ecx" :: "r"(&prev->regs));
    asm("mov %0, %%edx" :: "r"(&next->regs));
    asm("mov %ebp, (%ecx)");
    asm("mov %esp, 4(%ecx)");
    asm("call read_eip");
    asm("cmp $0xFFFFFFAC, %eax");
    asm("je warped_to_task");
    asm("mov %eax, 8(%ecx)");

    asm("mov (%edx), %ebp");
    asm("mov 4(%edx), %esp");
    asm("mov 8(%edx), %ecx");
    asm("mov $0xFFFFFFAC, %eax");
    asm("jmp *%ecx");

    asm("warped_to_task:");
    asm("popf");
    asm("popal");
    sti();
}
