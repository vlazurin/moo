#include "irq.h"
#include "signal.h"
#include "debug.h"
#include "mm.h"
#include "timer.h"
#include "vfs.h"
#include "task.h"
#include "errno.h"
#include <stddef.h>

static int syscall_write(file_descriptor_t fd, char *buf, uint32_t len)
{
    debug("[syscall] (PID: %i) write fd %i\n", get_pid(), fd);
    len = sys_write(fd, buf, len);
    return len;
}

static int syscall_read(file_descriptor_t fd, char *buf, int len)
{
    debug("[syscall] read fd %i\n", fd);
    len = sys_read(fd, buf, len);
    if (len < 0) {
        debug("read return -1\n");
        return -1;
    }
    debug("read return %i, string: %s\n", len, buf);
    return len;
}

static int syscall_fcntl(file_descriptor_t fd, uint32_t cmd, uint32_t arg)
{
    debug("[syscall] fcntl\n");
    return fcntl(fd, cmd, arg);
}

static int syscall_stat(char *path, struct stat *stat)
{
    debug("[syscall] stat: %s\n", path);
    return stat_fs(path, stat);
}

static int syscall_mkdir(char *path, mode_t mode)
{
    debug("[syscall] mkdir: %s\n", path);
    return mkdir("/test");
}

static int syscall_open(char *path, int flags, mode_t mode)
{
    debug("[syscall] open : %s %i %i\n", path, flags, mode);
    return sys_open(path);
}

static int syscall_getgid()
{
    debug("[syscall] getgid\n");
    return 12345;
}

static int syscall_brk(uint32_t addr)
{
    debug("[syscall] brk %h\n", addr);
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

static void syscall_exit()
{
    debug("[syscall] exit PID: %i\n", get_pid());
    current_process->state = PROCESS_TERMINATED;
    force_task_switch();
}

static int syscall_fork(struct regs *r)
{
    debug("[syscall] fork, PID: %i\n", get_pid());
    int result = fork(r);
    return result;
}
extern int execve(char *path, char **argv);
static int syscall_execve(struct regs *r)
{
    debug("execve: %s %h\n", (char*)r->ebx, (void*)r->ecx);
    debug("EIP: %h\n", r->eip);
    return execve((void*)r->ebx, (void*)r->ecx);
}

static int syscall_chdir(struct regs *r)
{
    debug("chdir: %s\n", (char*)r->ebx);
    return chdir((char*)r->ebx);
}

int syscall_tcgetpgrp(struct regs *r)
{
    debug("tcgetpgrp %i\n", r->ebx);
    return 0;
}

int syscall_tcsetpgrp(struct regs *r)
{
    debug("tcsetpgrp %i %i\n", r->ebx, r->ecx);
    return 0;
}

int syscall_setpgrp(struct regs *r)
{
    debug("setpgrp %i %i\n", r->ebx, r->ecx);
    int pid = r->ebx > 0 ? r->ebx : get_pid();
    int pgid = r->ecx > 0 ? r->ecx : current_process->group_id;

    struct process *p = proc_by_id(pid);
    if (p == NULL) {
        return -ESRCH;
    }
    p->group_id = pgid;

    return 0;
}

int syscall_sigaction(struct regs *r)
{
    debug("[syscall] sigaction\n");
    int signum = r->ebx;
    struct sigaction *new = (struct sigaction*)r->ecx;
    struct sigaction *old = (struct sigaction*)r->edx;

    if (old != NULL) {
        *old = current_process->signals[signum];
    }

    if (new == NULL) {
        current_process->signals[signum].sa_handler = SIG_DFL;
        current_process->signals[signum].sa_flags = 0;
        current_process->signals[signum].sa_mask = 0;
    } else {
        if (new->sa_flags & SA_SIGINFO) {
            debug("system doesn't support SA_SIGINFO\n");
            return -EINVAL;
        }
        current_process->signals[signum].sa_handler = new->sa_handler;
        current_process->signals[signum].sa_flags = new->sa_flags;
        current_process->signals[signum].sa_mask = new->sa_mask;
    }

    debug("signal %i has new handler %h\n", signum, current_process->signals[signum].sa_handler);
    return 0;
}

void handle_syscall_routine(struct regs *r)
{
    //debug("[syscall] eax: %h\n", r->eax);
    int result = -1;
    switch (r->eax)
    {
        case 0x2:
            result = syscall_fork(r);
        break;
        case 0x04:
            result = syscall_write(r->ebx, (char*)r->ecx, r->edx);
        break;
        case 0x05:
            result = syscall_open((char*)r->ebx, r->ecx, r->edx);
        break;
        case 0x3:
            result = syscall_read(r->ebx, (char*)r->ecx, r->edx);
        break;
        case 0x12:
            result = syscall_stat((char*)r->ebx, (void*)r->ecx);
        break;
        case 0x1C:
            debug("fstat\n");
            result = (-1);
        break;
        case 0x14:
            debug("getpid\n");
            result = get_pid();
        break;
        case 1:
            syscall_exit();
        break;
        case 0xb:
            result = syscall_execve(r);
        break;
        case 0x0c:
            result = syscall_chdir(r);
        break;
        case 0xc8:
            result = syscall_getgid();
        break;
        case 0x2d:
            result = syscall_brk(r->ebx);
        break;
        case 0x89:
            result = syscall_tcsetpgrp(r);
        break;
        case 0x90:
            result = syscall_tcgetpgrp(r);
        break;
        case 0x91:
            result = syscall_setpgrp(r);
        break;
        case 0x37:
            result = syscall_fcntl(r->ebx, r->ecx, r->edx);
        break;
        case 0x43:
            result = syscall_sigaction(r);
        break;
        case 0x27:
            result = syscall_mkdir((char*)r->ebx, r->ecx);
        break;
        case 0x72:
            debug("wait3 %h %h %h\n", r->ebx, r->ecx, r->edx);
            result = wait_pid(-1, (int*)r->ebx, r->ecx);
        break;
        case 0x48:
            debug("sigsuspend %i\n", r->ebx);
            result = 0;
        break;
        default:
            debug("[syscall] unknown (eax: %h, ebx: %h, ecx: %h, edx: %h)\n", r->eax, r->ebx, r->ecx, r->edx);
        break;
    }
    debug("syscall (%i)(%i) ret value %h\n", r->eax, get_pid(), result);
    r->eax = result;
}

void setup_syscalls()
{
    set_irq_handler(0x80, handle_syscall_routine);
}
