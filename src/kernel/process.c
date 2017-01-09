#include "tasking.h"
#include "mm.h"
#include "elf.h"
#include "vfs.h"
#include "debug.h"

void open_process_std();
extern page_directory_t *page_directory;

static void process_bootstrap(char *path)
{
    void (*entry_point)() = 0;
    load_elf(path, (void**)&entry_point);

    if (entry_point == 0)
    {
        return;
    }
    open_process_std();
    set_fg_process(get_pid());
    entry_point();
}

uint8_t exec(char *path)
{
    process_t *p = create_process(&process_bootstrap, path);
    schedule_process(p);
    return 0;
}

void execve()
{
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
    
    process_bootstrap("sss");
}
