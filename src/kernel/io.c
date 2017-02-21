#include "vfs.h"
#include "task.h"
#include "log.h"
#include "tty.h"

file_descriptor_t stdin;
file_descriptor_t stdout;

static void process_input(void *params)
{
    file_descriptor_t fd = sys_open("/dev/kb", 0);
    file_descriptor_t tmp = sys_open("/dev/screen", 0);
    if (fd < 0 || tmp < 0) {
        log(KERN_FATAL, "can't init input\n");
        hlt();
    }
    uint32_t count;
    char buffer[1];
    while(1)
    {
        count = sys_read(fd, buffer, 1);
        sys_write(stdin, buffer, count);
        sys_write(tmp, buffer, count);
    }
}

static void process_output(void *params)
{
    file_descriptor_t fd = sys_open("/dev/screen", 0);
    uint32_t count;
    while(1)
    {
        char buffer[100];
        count = sys_read(stdout, buffer, 100);
        sys_write(fd, buffer, count);
    }
}

void init_io()
{
    stdin = create_pty("stdin");
    stdout = create_pty("stdout");
    start_thread(&process_input, 0);
    start_thread(&process_output, 0);
}

void open_process_std()
{
    file_descriptor_t fd = sys_open("/dev/stdin", 0);
    if (fd < 0) {
        log(KERN_FATAL, "can't open stdin\n");
    }
    fd = sys_open("/dev/stdout", 0);
    if (fd < 0) {
        log(KERN_FATAL, "can't open stdout\n");
    }
    fd = sys_open("/dev/stdout", 0);
    if (fd < 0) {
        log(KERN_FATAL, "can't open stderr\n");
    }
}
