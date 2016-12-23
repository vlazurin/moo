#ifndef H_TTY
#define H_TTY

#include "vfs.h"
#include "buffer.h"

#define MAX_PTY_STR_LENGTH 4096
#define MAX_PTY_COUNT 100

#define PTY_FLAG_SLAVE_OPEN (1 << 0)
#define PTY_FLAG_MASTER_OPEN (1 << 1)
#define PTY_FLAG_FG_ONLY (1 << 2)

typedef struct pty
{
    vfs_node_t *master;
    vfs_node_t *slave;
    char input_buffer[MAX_PTY_STR_LENGTH];
    uint32_t input_buffer_pos;
    buffer_t *in;
    buffer_t *out;
    uint32_t flags;
} pty_t;

file_descriptor_t create_pty(char *slave_name);

#endif
