#include "irq.h"
#include "signal.h"
#include "log.h"
#include "mm.h"
#include "timer.h"
#include "vfs.h"
#include "task.h"
#include "errno.h"
#include "liballoc.h"
#include "string.h"
#include "syscalls.h"
#include <stddef.h>

typedef int (*syscall_handler)(int a, int b, int c);

static int syscall_close(file_descriptor_t fd)
{
    return -1;
    return sys_close(fd);
}

static int syscall_getgid()
{
    return 12345;
}

static int syscall_brk(uint32_t addr)
{
    log(KERN_DEBUG, "PID %i requested brk %x\n", get_pid(), addr);
    if (addr == 0 || (uint32_t)current_process->brk > addr)
    {
        return (uint32_t)current_process->brk;
    }

    uint32_t current = (uint32_t)current_process->brk;
    uint32_t new = PAGE_ALIGN(addr);
    while(current < new)
    {
        map_virtual_to_physical(current, alloc_physical_page());
        current += 0x1000;
    }

    return addr;
}

static int syscall_tcsetpgrp()
{
    return 0;
}

static int syscall_setpgrp()
{
    int pid = current_process->user_regs->ebx > 0 ? current_process->user_regs->ebx : get_pid();
    int pgid = current_process->user_regs->ecx > 0 ? current_process->user_regs->ecx : current_process->group_id;

    struct process *p = proc_by_id(pid);
    if (p == NULL) {
        return -ESRCH;
    }
    p->group_id = pgid;

    return 0;
}

static int syscall_getpgrp()
{
    return 12345;
    return current_process->group_id;
}

int syscall_sigsuspend()
{
    return -EINTR;
}

int syscall_debug(char *str, int len)
{
    char *buf = kmalloc(len + 1);
    memcpy(buf, str, len);
    buf[len] = '\0';
    log(KERN_DEBUG, buf);
    kfree(buf);
    return 0;
}

static int syscall_sigaction()
{
    return 0;
    int signum = current_process->user_regs->ebx;
    struct sigaction *new = (struct sigaction*)current_process->user_regs->ecx;
    struct sigaction *old = (struct sigaction*)current_process->user_regs->edx;

    if (old != NULL) {
        *old = current_process->signals[signum];
    }

    if (new == NULL) {
        current_process->signals[signum].sa_handler = SIG_DFL;
        current_process->signals[signum].sa_flags = 0;
        current_process->signals[signum].sa_mask = 0;
    } else {
        if (new->sa_flags & SA_SIGINFO) {
            log(KERN_INFO, "unsupported syscall_sigaction flag SA_SIGINFO\n");
            return -EINVAL;
        }
        current_process->signals[signum].sa_handler = new->sa_handler;
        current_process->signals[signum].sa_flags = new->sa_flags;
        current_process->signals[signum].sa_mask = new->sa_mask;
    }
    return 0;
}

static void syscall_exit()
{
    stop_process();
    force_task_switch();
}

static void *syscall_table[] = {
    [SYSCALL_EXIT] = syscall_exit,
    [SYSCALL_FORK] = fork,
    [SYSCALL_READ] = sys_read,
    [SYSCALL_WRITE] = sys_write,
    [SYSCALL_OPEN] = sys_open,
    [SYSCALL_CLOSE] = syscall_close,
    [SYSCALL_BRK] = syscall_brk,
    [SYSCALL_STAT] = stat_fs,
    [SYSCALL_GETPID] = get_pid,
    [SYSCALL_EXECVE] = execve,
    [SYSCALL_CHDIR] = chdir,
    [SYSCALL_GETGID] = syscall_getgid,
    [SYSCALL_TCSETPGRP] = syscall_tcsetpgrp,
    [SYSCALL_LSEEK] = lseek,
    [SYSCALL_TCGETPGRP] = 0,
    [SYSCALL_SETPGRP] = syscall_setpgrp,
    [SYSCALL_FCNTL] = fcntl,
    [SYSCALL_GETPGRP] = syscall_getpgrp,
    [SYSCALL_SIGACTION] = syscall_sigaction,
    [SYSCALL_MKDIR] = mkdir,
    [SYSCALL_WAITPID] = wait_pid,
    [SYSCALL_SIGSUSPEND] = syscall_sigsuspend,
    [SYSCALL_DEBUG] = syscall_debug,
};

static uint32_t syscall_table_size = sizeof(syscall_table) / sizeof(void*);

void handle_syscall_routine(struct regs *r)
{
    if (r->eax > syscall_table_size || syscall_table[r->eax] == NULL) {
        log(KERN_DEBUG, "unknown syscall (eax: %x, ebx: %x, ecx: %x, edx: %x)\n", r->eax, r->ebx, r->ecx, r->edx);
        r->eax = -EINVAL;
    } else {
        current_process->user_regs = r;
        syscall_handler handler = syscall_table[r->eax];
        r->eax = handler(r->ebx, r->ecx, r->edx);
    }
}

void setup_syscalls()
{
    set_irq_handler(0x80, handle_syscall_routine);
}
