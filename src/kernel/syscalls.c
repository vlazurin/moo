#include "irq.h"
#include "signal.h"
#include "log.h"
#include "mm.h"
#include "timer.h"
#include "shm.h"
#include "vfs.h"
#include "task.h"
#include "errno.h"
#include "liballoc.h"
#include "string.h"
#include "tty.h"
#include "syscalls.h"
#include <stddef.h>

typedef int (*syscall_handler)(int a, int b, int c);

/*
 * TODO: implement parameters check. Currently handle_syscall_routine() calls VFS functions directly... it's not good and
 * can crash a kernel
 */

static int syscall_brk(uint32_t addr)
{
    log(KERN_DEBUG, "PID %i requested brk %x\n", get_pid(), addr);
    if (addr == 0 || (uint32_t)current_process->brk > addr || addr >= USERSPACE_SHARED_MEM)
    {
        return (uint32_t)current_process->brk;
    }

    uint32_t current = (uint32_t)current_process->brk;
    uint32_t new = PAGE_ALIGN(addr);
    while(current < new)
    {
        map_virtual_to_physical(current, alloc_physical_page(), 0);
        current += 0x1000;
    }

    return addr;
}

static int syscall_tcsetpgrp(file_descriptor_t fd, int pgrp)
{
    return 0;
    vfs_file_t *file = get_file(fd);
    if (file == NULL) {
        return -ENOENT;
    }
    // TODO: add check for TTY
    pty_t *pty = file->node->obj;
    pty->ct_group_id = pgrp;
    return 0;
}

static int syscall_tcgetpgrp(file_descriptor_t fd)
{
    return 12345;
    vfs_file_t *file = get_file(fd);
    if (file == NULL) {
        return -ENOENT;
    }
    // TODO: add check for TTY
    pty_t *pty = file->node->obj;
    return pty->ct_group_id;
}

static int syscall_setpgrp()
{
    int pid = current_thread->user_regs->ebx > 0 ? current_thread->user_regs->ebx : get_pid();
    int pgid = current_thread->user_regs->ecx > 0 ? current_thread->user_regs->ecx : current_process->group_id;

    return set_proc_group(pid, pgid);
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
    int signum = current_thread->user_regs->ebx;
    struct sigaction *new = (struct sigaction*)current_thread->user_regs->ecx;
    struct sigaction *old = (struct sigaction*)current_thread->user_regs->edx;

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

static int syscall_getppid()
{
    return current_process->parent_id;
}

static int syscall_closedir()
{
    struct DIR *d = (void*)current_thread->user_regs->ebx;
    return sys_close(d->fd);
}

static int syscall_readdir()
{
    struct DIR *d = (void*)current_thread->user_regs->ebx;
    int i = sys_readdir(d->fd, &d->ent);
    if (i == 0) {
        d->cur_entry++;
    }
    return i;
}

static int syscall_opendir()
{
    char *p = (void*)current_thread->user_regs->ebx;
    return sys_open(p, 0);
}

static int syscall_getegid()
{
    return get_gid();
}

static int syscall_geteuid()
{
    return 0;
}

static int syscall_getuid()
{
    return 0;
}

static int syscall_shm_map()
{
    return shm_map((char*)current_thread->user_regs->ebx);
}

static int syscall_shm_alloc()
{
    return shm_alloc((char*)current_thread->user_regs->ebx, (uint32_t)current_thread->user_regs->ecx, NULL, 0);
}

static int syscall_shm_get_addr()
{
    uintptr_t addr = shm_get_addr((char*)current_thread->user_regs->ebx);
    if (addr == 0) {
        return -EINVAL;
    }
    uintptr_t *ptr = (uintptr_t*)current_thread->user_regs->ecx;
    *ptr = addr;
    return 0;
}

static void *syscall_table[] = {
    [SYSCALL_EXIT] = stop_process,
    [SYSCALL_FORK] = fork,
    [SYSCALL_READ] = sys_read,
    [SYSCALL_WRITE] = sys_write,
    [SYSCALL_OPEN] = sys_open,
    [SYSCALL_CLOSE] = sys_close,
    [SYSCALL_BRK] = syscall_brk,
    [SYSCALL_STAT] = stat_fs,
    [SYSCALL_GETPID] = get_pid,
    [SYSCALL_EXECVE] = execve,
    [SYSCALL_CHDIR] = chdir,
    [SYSCALL_GETGID] = get_gid,
    [SYSCALL_TCSETPGRP] = syscall_tcsetpgrp,
    [SYSCALL_LSEEK] = lseek,
    [SYSCALL_TCGETPGRP] = syscall_tcgetpgrp,
    [SYSCALL_SETPGRP] = syscall_setpgrp,
    [SYSCALL_FCNTL] = fcntl,
    [SYSCALL_GETPGRP] = syscall_getpgrp,
    [SYSCALL_SIGACTION] = syscall_sigaction,
    [SYSCALL_MKDIR] = mkdir,
    [SYSCALL_WAITPID] = wait_pid,
    [SYSCALL_SIGSUSPEND] = syscall_sigsuspend,
    [SYSCALL_DEBUG] = syscall_debug,
    [SYSCALL_GETPPID] = syscall_getppid,
    [SYSCALL_GETEGID] = syscall_getegid,
    [SYSCALL_GETEUID] = syscall_geteuid,
    [SYSCALL_GETUID] = syscall_getuid,
    [SYSCALL_OPENDIR] = syscall_opendir,
    [SYSCALL_READDIR] = syscall_readdir,
    [SYSCALL_CLOSEDIR] = syscall_closedir,
    [SYSCALL_SHM_MAP] = syscall_shm_map,
    [SYSCALL_SHM_GET_ADDR] = syscall_shm_get_addr,
    [SYSCALL_SHM_ALLOC] = syscall_shm_alloc,
    [0xce] = dup2
};

static uint32_t syscall_table_size = sizeof(syscall_table) / sizeof(void*);

void handle_syscall_routine(struct regs *r)
{
    if (r->eax > syscall_table_size || syscall_table[r->eax] == NULL) {
        //log(KERN_DEBUG, "unknown syscall (eax: %x, ebx: %x, ecx: %x, edx: %x)\n", r->eax, r->ebx, r->ecx, r->edx);
        r->eax = -EINVAL;
    } else {
        //log(KERN_DEBUG, "(PID: %i) syscall (eax: %i, ebx: %x, ecx: %x, edx: %x)\n", get_pid() ,r->eax, r->ebx, r->ecx, r->edx);
        current_thread->user_regs = r;
        syscall_handler handler = syscall_table[r->eax];
        r->eax = handler(r->ebx, r->ecx, r->edx);
        //log(KERN_DEBUG, "%i\n", r->eax);
    }
}

void setup_syscalls()
{
    set_irq_handler(0x80, handle_syscall_routine);
}
