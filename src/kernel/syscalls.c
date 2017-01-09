#include "irq.h"
#include "debug.h"
#include "mm.h"
#include "timer.h"
#include "vfs.h"
#include "tasking.h"

static int syscall_write(file_descriptor_t fd, char *buf, uint32_t len)
{
    debug("[syscall] write fd %i\n", fd);
    len = sys_write(fd, buf, len);
    return len;
}

static int syscall_read(file_descriptor_t fd, char *buf, uint32_t len)
{
    debug("[syscall] read fd %i\n", fd);
    len = sys_read(fd, buf, len);
    return len;
}

static int syscall_fcntl(file_descriptor_t fd, uint32_t cmd, uint32_t arg)
{
    debug("[syscall] fcntl\n");
    return fcntl(fd, cmd, arg);
}

static int syscall_stat(char *path, struct stat *stat)
{
    debug("[syscall] stat\n");
    return stat_fs(path, stat);
}

static int syscall_mkdir(char *path, mode_t mode)
{
    debug("[syscall] mkdir: %s\n", path);
    return mkdir("/test");
}

static int syscall_getgid()
{
    debug("[syscall] getgid\n");
    return 12345;
}

static int syscall_brk(uint32_t addr)
{
    debug("[syscall] brk\n");
    if (addr == 0 || (uint32_t)get_curent_proccess()->brk > addr)
    {
        return (uint32_t)get_curent_proccess()->brk;
    }

    uint32_t current = (uint32_t)get_curent_proccess()->brk;
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
    while(1){};
}

static int syscall_fork(struct regs *r)
{
    debug("[syscall] fork, PID: %i\n", get_pid());
    int result = fork(r);
    debug("fork result: %i\n", result);
    return result;
}
extern void execve();
static int syscall_execve(struct regs *r)
{
    debug("execve: %s\n", (void*)r->ebx);
    execve();
    return 0;
}

void handle_syscall_routine(struct regs *r)
{
    //debug("[syscall] eax: %h\n", r->eax);
    uint32_t result = -1;
    switch (r->eax)
    {
        case 0x2:
            result = syscall_fork(r);
        break;
        case 0x04:
            result = syscall_write(r->ebx, (char*)r->ecx, r->edx);
        break;
        case 0x3:
            result = syscall_read(r->ebx, (char*)r->ecx, r->edx);
        break;
        case 0x12:
            result = syscall_stat((char*)r->ebx, (void*)r->ecx);
        break;
        case 0x14:
            result = get_pid();
        break;
        case 1:
            syscall_exit();
        break;
        case 0xb:
            result = syscall_execve(r);
        break;
        case 0xc8:
            result = syscall_getgid();
        break;
        case 0x2d:
            result = syscall_brk(r->ebx);
        break;
        case 0x37:
            result = syscall_fcntl(r->ebx, r->ecx, r->edx);
        break;
        case 0x27:
            result = syscall_mkdir((char*)r->ebx, r->ecx);
        break;
        default:
            debug("[syscall] unknown (eax: %h, ebx: %h, ecx: %h, edx: %h)\n", r->eax, r->ebx, r->ecx, r->edx);
        break;
    }

    r->eax = result;
}

void setup_syscalls()
{
    set_irq_handler(0x80, handle_syscall_routine);
}
