#include "interrupts.h"
#include "debug.h"
#include "mm.h"
#include "timer.h"
#include "vfs.h"
#include "tasking.h"

static int syscall_write(file_descriptor_t fd, char *buf, uint32_t len)
{
    debug("[syscall] write\n");
    len = sys_write(1, buf, len);
    return len;
}

static int syscall_read(file_descriptor_t fd, char *buf, uint32_t len)
{
    debug("[syscall] read\n");
    len = read_file(0, buf, len);
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
    debug("[syscall] exit\n");
    while(1){};
}

static int syscall_fork()
{
    debug("[syscall] fork, PID: %i\n", get_pid());
    int result = fork();
    debug("fork result: %i\n", result);
    return result;
}

SYSCALL_HANDLER(handle_syscall_routine)
{
    sti();
    //debug("[syscall] eax: %h, ebx: %h, ecx: %h, edx: %h\n", syscall_num, param1, param2, param3);
    uint32_t result = -1;

    switch (syscall_num)
    {
        case 0x2:
            result = syscall_fork();
            debug("fork res %i\n", result);
        break;
        case 0x04:
            result = syscall_write(param1, (char*)param2, param3);
        break;
        case 0x3:
            result = syscall_read(param1, (char*)param2, param3);
        break;
        case 0x12:
            result = syscall_stat((char*)param1, (void*)param2);
        break;
        case 0x14:
            result = get_pid();
        break;
        case 0xb:
            syscall_exit();
        break;
        case 0xc8:
            result = syscall_getgid();
        break;
        case 0x2d:
            result = syscall_brk(param1);
        break;
        case 0x37:
            result = syscall_fcntl(param1, param2, param3);
        break;
        case 0x27:
            result = syscall_mkdir((char*)param1, param2);
        break;
        default:
            debug("[syscall] unknown (eax: %h, ebx: %h, ecx: %h, edx: %h)\n", syscall_num, param1, param2, param3);
        break;
    }

    return result;
}

void setup_syscalls()
{
    set_interrupt_gate(0x80, &handle_syscall_routine, 0x08, 0x8E);
}
