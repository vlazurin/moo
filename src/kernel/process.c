#include "tasking.h"
#include "elf.h"
#include "vfs.h"
#include "debug.h"

void open_process_std();

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
