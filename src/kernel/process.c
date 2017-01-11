#include "tasking.h"
#include "string.h"
#include "liballoc.h"
#include "mm.h"
#include "elf.h"
#include "vfs.h"
#include "debug.h"

void open_process_std();
extern page_directory_t *page_directory;

static void process_bootstrap(char *path)
{
    void (*entry_point)(int argc, char *argv[]) = 0;
    load_elf(path, (void**)&entry_point);

    if (entry_point == 0)
    {
        return;
    }
    open_process_std();
    set_fg_process(get_pid());
    char **params = 0;
    params = kmalloc(4);
    params[0] = path;
    entry_point(1, params);
}

uint8_t exec(char *path)
{
    process_t *p = create_process(&process_bootstrap, path);
    schedule_process(p);
    return 0;
}

void execve(char *path, char **argv)
{
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
    for(uint32_t i = 1; i < 0x387; i++) {
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

    void (*entry_point)(int argc, char *argv[]) = 0;
    load_elf(path_tmp, (void**)&entry_point);
    kfree(path_tmp);
    if (entry_point == 0)
    {
        debug("load_elf failed\n");
        return;
    }

    open_process_std();
    if (get_fg_process_pid() == get_pid()) {
        set_fg_process(get_pid());
    }
    //debug("%i %s %s\n", count, params[0], params[1]);
    entry_point(argc, params);
}
