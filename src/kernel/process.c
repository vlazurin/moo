#include "task.h"
#include "string.h"
#include "liballoc.h"
#include "mm.h"
#include "elf.h"
#include "vfs.h"
#include "debug.h"
#include "errno.h"
#include <stdbool.h>

void open_process_std();
extern page_directory_t *page_directory;
int execve(char *path, char **argv);

static void process_bootstrap(char *path)
{
    char *params[2];
    params[0] = "-dash";
    params[1] = 0;
    execve(path, params);
}

uint8_t exec(char *path)
{
    struct process *p = create_process(&process_bootstrap, (uint32_t)path);
    schedule_process(p);
    return 0;
}

int execve(char *path, char **argv)
{
    struct stat st;
    int err = stat_fs(path, &st);
    if (err) {
        return err;
    }
    if (st.st_mode != S_IFREG) {
        return -ENOEXEC;
    }

    int argc = 0;
    while(argv[argc] != NULL) {
        argc++;
    }
    char **params = kmalloc(argc * 4);
    for(int i = 0; i < argc; i++) {
        params[i] = kmalloc(strlen(argv[i]) + 1);
        strcpy(params[i], argv[i]);
    }
    char *path_tmp = kmalloc(MAX_PATH_LENGTH);
    strcpy(path_tmp, path);

    // must start from 0, fix stupid hack in tasking.c create_process
    for(uint32_t i = 1; i < KERNEL_SPACE_START_PAGE_DIR; i++) {
        if (page_directory->directory[i] == 2) {
            continue;
        }
        for(uint32_t y = 0; y < 1024; y++) {
            if ((page_directory->pages[i][y] & 3) == 3) {
                //debug("free %h\n", (i * 1024 + y) * 0x1000);
                free_page((i * 1024 + y) * 0x1000);
            }
        }
    }

    void (*entry_point)(int argc, char *argv[], char *envp[]) = 0;
    load_elf(path_tmp, (void**)&entry_point);
    kfree(path_tmp);
    if (entry_point == 0)
    {
        debug("load_elf failed\n");
        return -ENOEXEC;
    }

    open_process_std();
//    if (get_fg_pid() == get_pid()) {
        set_fg_pid(get_pid());
//    }
    //debug("%i %s %s\n", count, params[0], params[1]);
    char *envp[] = {"PWD=/"};
    entry_point(argc, params, envp);
    assert(true, "something isn't ok with execve -> unreachable point");
    return 0;
}
