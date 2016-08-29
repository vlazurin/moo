#include "tasking.h"
#include "interrupts.h"
#include "liballoc.h"
#include "debug.h"
#include "string.h"
#include "mutex.h"
#include "pit.h"

uint8_t volatile task_switch_required = 0;
uint8_t tasking_enabled = 0;
process_t *processes;
process_t *current_process = 0;
thread_t *current_thread = 0;

void terminate_thread()
{
    cli();
    debug("thread %i terminated in process %i\n", current_thread->id, current_thread->process->id);
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
    memset((void*)thread, sizeof(thread_t), 0);
    thread->state = THREAD_STATE_RUNNING;
    thread->regs.eip = (uint32_t)&thread_header;
    thread->regs.ebp = (uint32_t)kmalloc(8192) + 8192;
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
    thread_t *last = current_thread;
    while(last->next != 0)
    {
        last = last->next;
    }
    last->next = thread;
    sti();
}

void init_tasking()
{
    process_t *process = kmalloc(sizeof(process_t));
    memset((void*)process, sizeof(process_t), 0);
    process->next_thread_id = 1;
    process->id = 0;

    thread_t *thread = kmalloc(sizeof(thread_t));
    memset((void*)thread, sizeof(thread_t), 0);
    thread->id = 0;
    thread->state = THREAD_STATE_RUNNING;
    thread->process = process;

    process->threads = thread;
    current_thread = thread;
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
    next = current_thread->next;
    prev = current_thread;
    while(1)
    {
        if (next == 0)
        {
            next = current_thread->process->threads;
        }

        if (next->state == THREAD_STATE_TERMINATED)
        {
            next = next->next;
            continue;
        }
        break;
    };

    current_thread = next;

    asm("pushal");
    asm("pushf");
    asm("mov %0, %%ecx" :: "r"(&prev->regs));
    asm("mov %0, %%edx" :: "r"(&next->regs));
    asm("mov %ebp, (%ecx)");
    asm("mov %esp, 4(%ecx)");
    asm("call read_eip");
    asm("cmp $0xFFFFFFFF, %eax");
    asm("je warped_to_task");
    asm("mov %eax, 8(%ecx)");

    asm("mov (%edx), %ebp");
    asm("mov 4(%edx), %esp");
    asm("mov 8(%edx), %ecx");
    asm("mov $0xFFFFFFFF, %eax");
    asm("jmp *%ecx");

    asm("warped_to_task:");
    asm("popf");
    asm("popal");
    sti();
}
