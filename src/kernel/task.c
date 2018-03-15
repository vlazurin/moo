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
#include "gdt.h"

extern void perform_task_switch(uint32_t eip, uint32_t ebp, uint32_t esp);
extern void return_to_userspace();
extern uint32_t read_eip();

extern page_directory_t *page_directory;

static struct process *process_list;

struct process *current_process = NULL;
struct thread *current_thread = NULL;
static struct process *fg_process = NULL;

uint8_t volatile task_switch_required = 0;
static mutex_t global_mutex = {0};
static int next_pid = 0;

static void stop_thread()
{
    assert((uint32_t)current_thread >= KERNEL_SPACE_ADDR);
    // report first, because no garanties that log() will be called after state change
    log(KERN_INFO, "thread %i stopped\n", current_thread->id);
    // very simple changes, mutex lock isn't needed
    current_thread->state = THREAD_STOPED;
    force_task_switch();
}

static void thread_header(void *entry_point, uint32_t arg)
{
    assert((uintptr_t)entry_point >= KERNEL_SPACE_ADDR);

    void (*func)(uint32_t) = entry_point;
    func(arg);
    stop_thread();
    log(KERN_FATAL, "thread %i isn't stopped, system is unstable", current_thread->id);
    hlt();
}

static struct thread *create_thread()
{
    struct thread *thread = kmalloc(sizeof(struct thread));
    memset((void*)thread, 0, sizeof(struct thread));
    thread->state = THREAD_RUNNING;
    thread->regs.eip = (uint32_t)&thread_header;
    thread->stack_mem = kmalloc(THREAD_STACK_SIZE);
    thread->regs.ebp = ALIGN((uintptr_t)thread->stack_mem + THREAD_STACK_SIZE - 16, 16);
    thread->regs.esp = thread->regs.ebp;
    return thread;
}

void stop_process()
{
    // stop all other threads, so nobody can work with files while current thread closes them
    // ref_count++ for thread isn't needed, thread can't be removed from list because of process mutex lock
    mutex_lock(&current_process->mutex);
    struct thread *iterator = current_process->threads;
    while(iterator != 0) {
        if (iterator != current_thread) {
            // very simple changes, mutex lock isn't needed
            iterator->state = THREAD_STOPED;
        }
        iterator = (struct thread*)iterator->list.next;
    }
    mutex_release(&current_process->mutex);

    // now only one thread is alive, so current_process can be changed without lock (or sys_close will cause deadlock)
    int err = 0;
    for(uint32_t i = 0; i < MAX_OPENED_FILES; i++) {
        if (current_process->files[i] != NULL) {
            err = sys_close(i);
            if (err) {
                debug("stop_process() -> sys_close(%i) error %i\n", i, err);
            }
        }
    }

    free_userspace();

    if (get_fg_pid() == get_pid()) {
        set_fg_pid(current_process->parent_id);
    }

    current_process->state = PROCESS_STOPED;
    force_task_switch();
}

static void failsafe_return()
{
    log(KERN_ERR, "failsafe_return() called, system is unstable");
    hlt();
}

void start_thread(void *entry_point, uint32_t arg)
{
    assert((uintptr_t)entry_point >= KERNEL_SPACE_ADDR);
    assert(current_process != NULL);
    // failsafe, probably it never will be false :)
    assert(current_process->next_thread_id < 0xFFFFFF00);

    struct thread *thread = create_thread();
    ref_inc(&thread->ref_count);
    PUSH_STACK(thread->regs.esp, arg);
    PUSH_STACK(thread->regs.esp, entry_point);
     // thread_header must kill thread, but it's good to have return address for failsafe
    PUSH_STACK(thread->regs.esp, &failsafe_return);

    thread->process = current_process;
    ref_inc(&current_process->ref_count);
    thread->id = __sync_fetch_and_add(&current_process->next_thread_id, 1);

    ref_inc(&thread->ref_count);
    cli();
    add_to_list(current_process->threads, thread);
    sti();
}

struct process *create_process(void *entry_point, uint32_t arg)
{
    struct process *process = kmalloc(sizeof(struct process));
    memset((void*)process, 0, sizeof(struct process));

    // failsafe, probably it never will be false :)
    assert(next_pid < 0x7FFFFFFF16);

    process->signals_queue = create_ring(100);

    process->id = __sync_fetch_and_add(&next_pid, 1);
    process->group_id = process->id;
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
    PUSH_STACK(thread->regs.esp, &stop_thread);

    thread->process = process;
    process->threads = thread;
    ref_inc(&thread->ref_count);
    ref_inc(&process->ref_count);

    return process;
}

void schedule_process(struct process *p)
{
    assert((uint32_t)p >= KERNEL_SPACE_ADDR && (uint32_t)p->threads >= KERNEL_SPACE_ADDR);
    p->state = PROCESS_RUNNING;

    ref_inc(&p->ref_count);
    cli();
    add_to_list(process_list, p);
    sti();
}

int get_p_pid()
{
    assert((uint32_t)current_process >= KERNEL_SPACE_ADDR);
    return current_process->parent_id;
}

int get_pid()
{
    assert((uint32_t)current_process >= KERNEL_SPACE_ADDR);
    // possible case when log() calls get_pid() before process 0 initialization
    if (current_process == NULL) {
        return -ESRCH;
    }
    return current_process->id;
}

int get_gid()
{
    assert((uint32_t)current_process >= KERNEL_SPACE_ADDR);
    return current_process->group_id;
}

void set_fg_pid(uint32_t pid)
{
    assert((uint32_t)process_list >= KERNEL_SPACE_ADDR);
    mutex_lock(&global_mutex);
    struct process *iterator = process_list;
    while(iterator != 0) {
        if (iterator->id == pid) {
            if (fg_process != NULL) {
                ref_dec(&fg_process->ref_count);
            }
            fg_process = iterator;
            ref_inc(&fg_process->ref_count);
            break;
        }
        iterator = (struct process*)iterator->list.next;
    }
    mutex_release(&global_mutex);
    log(KERN_INFO, "foreground PID %i\n", iterator->id);
}

uint32_t get_fg_pid()
{
    assert((uint32_t)fg_process >= KERNEL_SPACE_ADDR);
    return fg_process->id;
}

int fork()
{
    struct process *p = create_process(0, 0);
    p->parent_id = get_pid();

    mutex_lock(&current_process->mutex);

    p->group_id = current_process->group_id;
    strcpy(p->cur_dir, current_process->cur_dir);

    for(uint32_t i = 0; i < MAX_OPENED_FILES; i++) {
        if (current_process->files[i] != NULL) {
            mutex_lock(&current_process->files[i]->mutex);
            p->files[i] = kmalloc(sizeof(vfs_file_t));
            memcpy(p->files[i], current_process->files[i], sizeof(vfs_file_t));
            mutex_release(&current_process->files[i]->mutex);
            p->files[i]->pid = p->id;
        }
    }

    mutex_release(&current_process->mutex);

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
                map_virtual_to_physical((uint32_t)buffer, phys, 0);
                memcpy(buffer, (void*)((i * 0x400 + y) * 0x1000), 0x1000);
                p->page_dir->pages[i][y] = phys | 7;
            } else {
                p->page_dir->pages[i][y] = 2;
            }
        }
    }
    map_virtual_to_physical((uint32_t)buffer, buffer_phys_back, 0);
    kfree(buffer_chunk);

    memcpy(p->threads[0].stack_mem + KERNEL_STACK_SIZE - sizeof(struct regs), current_thread->user_regs, sizeof(struct regs));
    p->threads[0].regs.esp = (uint32_t)p->threads[0].stack_mem + KERNEL_STACK_SIZE - sizeof(struct regs);
    p->threads[0].regs.ebp = p->threads[0].regs.esp;
    p->threads[0].regs.eip = (uint32_t)&return_to_userspace;
    struct regs *new = (struct regs*)p->threads[0].regs.esp;
    new->eax = 0;
    int child_pid = p->id;
    p->state = PROCESS_RUNNING;
    p->threads->state = THREAD_RUNNING;

    ref_inc(&p->ref_count);
    cli();
    add_to_list(process_list, p);
    sti();
    return child_pid;
}

int wait_pid(int pid, int *status, int options)
{
    while(1) {
        if (pid == -1) {
            mutex_lock(&global_mutex);
            struct process *iterator = process_list;
            while(iterator != NULL) {
                if (iterator->parent_id == current_process->id && iterator->state == PROCESS_DEAD) {
                    cli();
                    // no ref_dec here, because we also need to do ref_inc (function will keep pointer after global_mutex release)
                    delete_from_list((void*)&process_list, iterator);
                    sti();
                    break;
                }
                iterator = (struct process*)iterator->list.next;
            }
            mutex_release(&global_mutex);
            if (iterator == NULL) {
                if (options & WNOHANG) {
                    return -ECHILD;
                }
                continue;
            } else {
                //TODO: return correct status
                *status = 0x0;
                int id = iterator->id;
                debug("wait_pid: iterator ref_count is %i\n", iterator->ref_count);
                if (ref_dec(&iterator->ref_count) == 0) {
                    debug("wait_pid is trying to free dead process %i\n", id);
                    kfree(iterator);
                }

                return id;
            }
        }
        force_task_switch();
    }
}

int set_proc_group(int pid, int group_id)
{
    mutex_lock(&global_mutex);
    struct process *iterator = process_list;
    while(iterator != NULL) {
        if (iterator->id == pid) {
            iterator->group_id = group_id;
            mutex_release(&global_mutex);
            return 0;
        }
        iterator = (struct process*)iterator->list.next;
    }
    mutex_release(&global_mutex);
    return -ESRCH;
}

static void killer()
{
    while(true) {
        mutex_lock(&global_mutex);
        struct process *iterator = process_list;
        while(iterator != NULL) {
            mutex_lock(&iterator->mutex);
            struct thread *thread_iterator = iterator->threads;
            while(thread_iterator != NULL) {
                if (thread_iterator->state == THREAD_STOPED || iterator->state == PROCESS_STOPED) {
                    thread_iterator->state = THREAD_STOPED;
                    struct thread *tmp = thread_iterator;
                    thread_iterator = (struct thread*)thread_iterator->list.next;
                    cli();
                    delete_from_list((void*)&iterator->threads, tmp);
                    sti();
                    if (ref_dec(&tmp->ref_count) == 0) {
                        kfree(tmp->stack_mem);
                        kfree(tmp);
                        ref_dec(&iterator->ref_count);
                    }
                    continue;
                }
                thread_iterator = (struct thread*)thread_iterator->list.next;
            }

            if (iterator->threads == NULL) {
                iterator->state = PROCESS_DEAD;
                kfree(iterator->page_dir_base);
            }

            mutex_release(&iterator->mutex);
            iterator = (struct process*)iterator->list.next;
        }
        mutex_release(&global_mutex);
        force_task_switch();
    }
}

void init_multitasking()
{
    struct process *process = kmalloc(sizeof(struct process));
    memset((void*)process, 0, sizeof(struct process));
    process->signals_queue = create_ring(100);
    process->state = PROCESS_RUNNING;
    process->next_thread_id = 1;
    process->id = __sync_fetch_and_add(&next_pid, 1);
    process->group_id = process->id;
    process->page_dir_base = (void*)PAGE_DIRECTORY_VIRTUAL;
    process->page_dir = (page_directory_t*)PAGE_DIRECTORY_VIRTUAL;
    strcpy(process->cur_dir, DEFAULT_DIR);

    struct thread *thread = kmalloc(sizeof(struct thread));
    memset((void*)thread, 0, sizeof(struct thread));
    thread->id = 0;
    thread->state = THREAD_RUNNING;
    thread->process = process;
    ref_inc(&process->ref_count);
    set_kernel_stack((uint32_t)thread->stack_mem + KERNEL_STACK_SIZE);

    process->threads = thread;
    ref_inc(&thread->ref_count);

    current_thread = thread;
    current_process = process;

    ref_inc(&current_process->ref_count);
    ref_inc(&current_thread->ref_count);

    process_list = process;
    start_thread(killer, 0);
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
    int c = 0;
    while(true) {
        if (c > 100) {
            debug("switch_task infinity loop\n");
            struct process *iterator = process_list;
            while(iterator != NULL) {
                struct thread *thread_iterator = iterator->threads;
                while(thread_iterator != NULL) {
                    debug("%i %i\n", iterator->id, thread_iterator->id);
                    thread_iterator = (struct thread*)thread_iterator->list.next;
                }
                iterator = (struct process*)iterator->list.next;
            }
            hlt();
        }

        if (ps == NULL) {
            ps = process_list;
            th = ps->threads;
            c++;
            continue;
        }
        if (th == NULL || ps->state != PROCESS_RUNNING) {
            ps = (struct process*)ps->list.next;
            th = ps->threads;
            c++;
            continue;
        }
        assert(th != NULL);
        if (th->state != THREAD_RUNNING) {
            th = (struct thread*)th->list.next;
            c++;
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

    ref_dec(&current_process->ref_count);
    ref_dec(&current_thread->ref_count);
    current_process = ps;
    current_thread = th;
    ref_inc(&current_process->ref_count);
    ref_inc(&current_thread->ref_count);
    set_kernel_stack((uint32_t)th->stack_mem + KERNEL_STACK_SIZE);
    if (current_thread->regs.eip == 0xc0100800) {
        hlt();
    }
    perform_task_switch(current_thread->regs.eip, current_thread->regs.ebp, current_thread->regs.esp);
}

struct event_data *get_event()
{
    struct event_data *packet = NULL;

    mutex_lock(&global_mutex);
    if (current_process->events != NULL) {
        packet = current_process->events;
        delete_from_list((void*)&current_process->events, packet);
    }
    mutex_release(&global_mutex);

    return packet;
}

int send_event(struct event_data *packet)
{
    mutex_lock(&global_mutex);

    FOR_EACH(p, process_list, struct process) {
        if (p->id == packet->target) {
            packet->sender = get_pid();
            push_in_list((void*)&p->events, packet);
            mutex_release(&global_mutex);
            return 0;
        }
    }

    mutex_release(&global_mutex);
    return -ENOENT;
}
