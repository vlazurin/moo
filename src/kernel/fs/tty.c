#include <stdbool.h>
#include "tty.h"
#include "errno.h"
#include "liballoc.h"
#include "string.h"
#include "stdlib.h"
#include "debug.h"
#include "system.h"
#include "task.h"

extern struct process *current_process;

static int slave_write(vfs_file_t *file, void *buf, uint32_t size, uint32_t *offset)
{
    pty_t *pty = file->node->obj;
    //debug("pty slave write\n");
    if (!(pty->flags & PTY_FLAG_SLAVE_OPEN))
    {
        debug("[pty] slave write ignored because pty is closed");
        return 0;
    }

    uint8_t reported = false;
    while ((pty->flags & PTY_FLAG_FG_ONLY) && get_fg_pid() != file->pid)
    {
        if (reported == false) {
            debug("[pty] slave write call from background process %i -> wait\n", get_fg_pid());
            reported = true;
        }
        force_task_switch();
    }

    uint32_t min = MIN(size, pty->out->get_free_space(pty->out));
    pty->out->add(pty->out, buf, min);
    return min;
}

static int master_write(vfs_file_t *file, void *buf, uint32_t size, uint32_t *offset)
{
    char *buffer = buf;
    pty_t *pty = file->node->obj;

    if (!(pty->flags & PTY_FLAG_SLAVE_OPEN))
    {
        //debug("[pty] master write ignored because pty is closed");
        return 0;
    }

    uint32_t accepted = 0;
    while(size > 0)
    {
        if (pty->input_buffer_pos == MAX_PTY_STR_LENGTH - 1) // 1 free byte for newline char, so master always is able to flush buffer
        {
            if (*buffer != '\n')
            {
                buffer++;
                goto next;
            }
        }

        switch (*buffer) {
            case '\b':
                if (pty->input_buffer_pos > 0)
                {
                    pty->input_buffer_pos--;
                }
                buffer++;
            break;
            case '\n':
                if (pty->input_buffer_pos > 0)
                {
                    pty->input_buffer[pty->input_buffer_pos++] = *buffer++;
                    uint8_t success = pty->in->add(pty->in, pty->input_buffer, pty->input_buffer_pos);
                    if (success != BUFFER_OK)
                    {
                        debug("[pty] flush to slave failed, buffer is full\n");
                    }

                    pty->input_buffer_pos = 0;
                }
            break;
            default:
                pty->input_buffer[pty->input_buffer_pos++] = *buffer++;
            break;
        }
        next:
            size--;
            accepted++;
    }
    return accepted;
}

static int master_read(vfs_file_t *file, void *buf, uint32_t size, uint32_t *offset)
{
    //debug("read from pty master\n");
    pty_t *pty = file->node->obj;
    uint32_t count = pty->in->get(pty->out, buf, size);
    while(count == 0)
    {
        force_task_switch();
        count = pty->in->get(pty->out, buf, size);
    }
    return count;
}

static int slave_read(vfs_file_t *file, void *buf, uint32_t size, uint32_t *offset)
{
    pty_t *pty = file->node->obj;

    while ((pty->flags & PTY_FLAG_FG_ONLY) && get_fg_pid() != file->pid)
    {
        debug("[pty] slave read call from background process -> wait\n");
        force_task_switch();
    }

    uint32_t count = pty->in->get_until(pty->in, buf, size, '\n');
    while(count == 0)
    {
        force_task_switch();
        count = pty->in->get_until(pty->in, buf, size, '\n');
    }

    return count;
}

static int slave_open(vfs_file_t *file, uint32_t flags)
{
    pty_t *pty = file->node->obj;
    //TODO: add mutex
    //TODO: open time counter... so 2+ app can use it
    /*if (pty->flags & PTY_FLAG_SLAVE_OPEN)
    {
        return -EBUSY;
    }*/

    pty->flags |= PTY_FLAG_SLAVE_OPEN;
    return 0;
}

static int slave_close(vfs_file_t *file)
{
    pty_t *pty = file->node->obj;
    pty->flags = pty->flags & ~PTY_FLAG_SLAVE_OPEN;
    return 0;
}

vfs_file_operations_t pty_slave_file_ops = {
    .open = &slave_open,
    .close = &slave_close,
    .write = &slave_write,
    .read = &slave_read
};

vfs_file_operations_t pty_master_file_ops = {
    .open = 0,
    .close = 0,
    .write = &master_write,
    .read = &master_read
};

file_descriptor_t create_pty(char *slave_name)
{
    pty_t *pty = kmalloc(sizeof(pty_t));
    memset(pty, 0, sizeof(pty_t));

    pty->flags = PTY_FLAG_FG_ONLY;

    pty->master = kmalloc(sizeof(vfs_node_t));
    memset(pty->master, 0, sizeof(vfs_node_t));
    pty->master->mode = S_IFCHR;
    pty->master->file_ops = &pty_master_file_ops;
    pty->master->obj = pty;
    strcpy(pty->master->name, "pty master");

    if (slave_name == 0)
    {
        // TODO: race condition here...
        char name[32] = "/dev/pty";
        uint32_t pos = strlen(name);
        for(uint32_t i = 0; i < MAX_PTY_COUNT; i++)
        {
            itoa(i, &name[pos], 10);
            int err = create_vfs_node(name, S_IFCHR, &pty_slave_file_ops, pty, &pty->slave);
            if (!err) {
                break;
            } else if (err == -EEXIST) {
                continue;
            } else {
                return err;
            }
        }
    }
    else
    {
        char name[32];
        sprintf(name, "/dev/%s", slave_name);
        int err = create_vfs_node(name, S_IFCHR, &pty_slave_file_ops, pty, &pty->slave);
        if (err) {
            // CLEANUP!!!
            return err;
        }
    }

    if (pty->slave == 0)
    {
        debug("No free pty slaves... seems something goes wrong!");
        hlt();
    }

    pty->in = create_buffer(MAX_PTY_STR_LENGTH);
    pty->out = create_buffer(MAX_PTY_STR_LENGTH);

    vfs_file_t *file = kmalloc(sizeof(vfs_file_t));
    memset(file, 0, sizeof(vfs_file_t));
    file->ops = pty->master->file_ops;
    file->node = pty->master;
    file->pid = current_process->id;

    for(uint32_t i = 0; i < MAX_OPENED_FILES; i++)
    {
        if (current_process->files[i] == 0)
        {
            current_process->files[i] = file;
            return i;
        }
    }

    pty->in->free(pty->in);
    pty->out->free(pty->out);
    kfree(pty);
    return -1;
}
