#include "task.h"
#include "string.h"
#include "liballoc.h"
#include "gdt.h"
#include "mm.h"
#include "elf.h"
#include "vfs.h"
#include "log.h"
#include "errno.h"
#include "tss.h"
#include <stdbool.h>

void open_process_std();
extern page_directory_t *page_directory;
extern void enter_userspace(uintptr_t location, uintptr_t stack);

static void process_bootstrap(char *path)
{
    char *params[2];
    params[0] = "-dash";
    params[1] = 0;
    char *envp[2];
    envp[0] = "PWD=/";
    envp[1] = 0;
    execve(path, params, envp);
}

uint8_t exec(char *path)
{
    struct process *p = create_process(&process_bootstrap, (uint32_t)path);
    schedule_process(p);
    return 0;
}

static int copy_params(char **arg, char **result)
{
    // count args count and required memory
    int argc = 0;
    int size = 0;
    while(arg[argc] != NULL) {
        size += strlen(arg[argc]) + 1; // +1 for EOS char
        argc++;
    }

    if (size + sizeof(uint32_t*) * argc > 0x1000) {
        return -E2BIG;
    }

    // copy params to tmp storage
    void *params = kmalloc(size);
    void *cur = params;
    for(uint32_t i = 0; i < argc; i++) {
        int length = strlen(arg[i]) + 1;
        memcpy(cur, arg[i], length);
        cur += length;
    }

    *result = params;

    return argc;
}

static char** make_params(char *params, int argc)
{
    char **arg = current_process->brk;
    uint32_t offset = 0;
    uint32_t phys = alloc_physical_page();
    map_virtual_to_physical((uint32_t)arg, phys);
    char *rows = (void*)arg + sizeof(char*) * (argc + 1); // +1 for empty NULL param
    for(uint32_t i = 0; i < argc; i++) {
        uint32_t size = strlen(params + offset) + 1;
        assert(((uint32_t)rows < (uint32_t)arg + 0x1000) && "execve params size > 0x1000, are you serious?");
        memcpy(rows, params + offset, size);
        arg[i] = rows;
        rows += size;
        offset += size;
    }
    arg[argc] = 0;
    return arg;
}

int execve(char *path, char **argv, char **envp)
{
    int err = check_elf(path);
    if (err) {
        return err;
    }

    char *argv_tmp = NULL;
    char *enpv_tmp = NULL;
    int argc = copy_params(argv, &argv_tmp);
    int envc = copy_params(envp, &enpv_tmp);

    // backup exec path
    char *path_back = kmalloc(MAX_PATH_LENGTH);
    strcpy(path_back, path);

    free_userspace();
    current_process->brk = NULL;
    // from now thread must be terminated if any errors, it can't return to userspace anymore

    uint32_t entry_point = 0;
    err = load_elf(path_back, (void**)&entry_point);
    kfree(path_back);
    if (err || entry_point == 0) {
        if (argv_tmp != NULL) kfree(argv_tmp);
        if (enpv_tmp != NULL) kfree(enpv_tmp);
        stop_process();
    }

    // setup userspace stack
    void* brk = current_process->brk;
    for(uint8_t i = 0; i < USERSPACE_STACK_SIZE / 0x1000; i++) {
        uint32_t page = alloc_physical_page();
        map_virtual_to_physical(USERSPACE_STACK + i * 0x1000, page);
    }
    // restore brk because it's modified by last map_virtual_to_physical calls
    current_process->brk = brk;

    argv = make_params(argv_tmp, argc);
    envp = make_params(enpv_tmp, envc);
    if (argv_tmp != NULL) kfree(argv_tmp);
    if (enpv_tmp != NULL) kfree(enpv_tmp);

    open_process_std();
    set_fg_pid(get_pid());

    set_kernel_stack((uint32_t)current_thread->stack_mem + KERNEL_STACK_SIZE);

    uint32_t user_stack = USERSPACE_STACK_TOP;

    PUSH_STACK(user_stack, envp);
    PUSH_STACK(user_stack, argv);
    PUSH_STACK(user_stack, argc);
    user_stack -= 4; // return address
    for(int i = 0; i < argc; i++) {
        log(KERN_DEBUG, "execve argv[%i] location %x, value \"%s\"\n", i, argv[i], argv[i]);
    }
    for(int i = 0; i < envc; i++) {
        log(KERN_DEBUG, "execve envp[%i] location %x, value \"%s\"\n", i, envp[i], envp[i]);
        if (strncmp(envp[i], "PWD=", 4) == 0) {
            strcpy(current_process->cur_dir, envp[i] + 4);
        }
    }

    enter_userspace((uint32_t)entry_point, user_stack);
    assert(false && "execve is broken (unreachable code executed)");
    while(true);
    return 0;
}
