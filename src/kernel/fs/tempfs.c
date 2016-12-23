#include "vfs.h"
#include "liballoc.h"
#include "system.h"
#include "string.h"
#include "debug.h"

vfs_node_t *create_root();
uint8_t create_node(vfs_node_t *node, char *path, mode_t mode, vfs_file_operations_t *file_ops, void *obj, vfs_node_t **out);
static uint8_t exist(vfs_node_t *node, char *path);
vfs_node_t* get_node(vfs_node_t *node, char *path);
static vfs_node_t *get_child(vfs_node_t *node, char *name);
static int stat_node(vfs_node_t *node, char *path, struct stat *buf);
//uint32_t list_nodes(vfs_node_t *node, void *buf, uint32_t count);

vfs_fs_operations_t tempfs_ops = {
    .create_root = &create_root
};

vfs_node_operations_t tempfs_node_ops = {
//    .list_nodes = &list_nodes,
    .exist = &exist,
    .create_node = &create_node,
    .get_node = &get_node,
    .stat = &stat_node
};

vfs_node_t *create_root()
{
    vfs_node_t *new = kmalloc(sizeof(vfs_node_t));
    memset(new, 0, sizeof(vfs_node_t));
    new->ops = &tempfs_node_ops;
    new->mode = S_IFDIR;
    strcpy(new->name, "tempfs_root");
    return new;
}

void init_tempfs()
{
    vfs_type_t *fs = kmalloc(sizeof(vfs_type_t));
    memset(fs, 0, sizeof(vfs_type_t));
    strcpy(fs->name, "tempfs");
    fs->ops = &tempfs_ops;
    register_fs(fs);
}

static int stat_node(vfs_node_t *node, char *path, struct stat *buf)
{
    char *token = strtok_r(path, "/", &path);
    if (token == NULL || strlen(token) == 0)
    {
        return VFS_BAD_PATH;
    }

    vfs_node_t *next = get_child(node, token);
    if (next == NULL)
    {
        return VFS_BAD_PATH;
    }

    if (strlen(path) > 0)
    {
        return next->ops->stat(next, path, buf);
    }

    memset(buf, 0, sizeof(struct stat));
    buf->st_blksize = VFS_BLOCK_SIZE;
    buf->st_blocks = ALIGN(node->size, VFS_BLOCK_SIZE) / VFS_BLOCK_SIZE;
    buf->st_size = node->size;
    buf->st_mode = node->mode;

    return 0;
}

static uint8_t exist(vfs_node_t *node, char *path)
{
    char *token = strtok_r(path, "/", &path);
    if (token == 0 || strlen(token) == 0)
    {
        return 0;
    }

    vfs_node_t *next = get_child(node, token);
    if (next == 0)
    {
        return 0;
    }

    if (strlen(path) > 0)
    {
        return next->ops->exist(next, path);
    }

    return 1;
}

static vfs_node_t *get_child(vfs_node_t *node, char *name)
{
    vfs_node_t *iterator = node->children;
    while(iterator != 0)
    {
        if (strcmp(iterator->name, name) == 0)
        {
            return iterator;
        }
        iterator = (vfs_node_t*)iterator->list.next;
    }

    return 0;
}

vfs_node_t* get_node(vfs_node_t *node, char *path)
{
    char *token = strtok_r(path, "/", &path);
    if (token == 0 || strlen(token) == 0)
    {
        return 0;
    }

    vfs_node_t *next = get_child(node, token);
    if (next == 0)
    {
        return 0;
    }

    if (strlen(path) > 0)
    {
        return next->ops->get_node(next, path);
    }

    return next;
}

/*
 * path - must be a copy
 */
uint8_t create_node(vfs_node_t *node, char *path, uint32_t mode, vfs_file_operations_t *file_ops, void *obj, vfs_node_t **out)
{
    char *token = strtok_r(path, "/", &path);
    if (token == 0 || strlen(token) == 0)
    {
        return VFS_BAD_PATH;
    }

    vfs_node_t *next = get_child(node, token);
    if (next == 0 && strlen(path) > 0)
    {
        return VFS_BAD_PATH;
    }

    if (strlen(path) > 0)
    {
        return next->ops->create_node(next, path, mode, file_ops, obj, out);
    }

    vfs_node_t *new = kmalloc(sizeof(vfs_node_t));
    memset(new, 0, sizeof(vfs_node_t));
    new->mode = mode;
    new->obj = obj;
    new->file_ops = file_ops;
    new->ops = node->ops;
    strcpy(new->name, token);
    add_to_list(node->children, new);
    if (out != 0)
    {
        *out = new;
    }
    return VFS_SUCCESS;
}

/*uint32_t list_nodes(vfs_node_t *node, void *buf, uint32_t count)
{
    vfs_node_t *iterator = node->children;
    dirent_t *ent = buf;
    uint32_t processed = 0;
    while(iterator != 0 && processed < count)
    {
        strcpy(ent->name, iterator->name);
        ent++;
        processed++;
        iterator = (vfs_node_t*)iterator->list.next;
    }

    return processed;
}*/
