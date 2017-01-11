#include "vfs.h"
#include "errno.h"
#include "liballoc.h"
#include "system.h"
#include "string.h"
#include "debug.h"

static struct vfs_super* read_super(struct ata_device *dev);
struct vfs_node *spawn_node(struct vfs_super *super, char *name, mode_t mode);
vfs_node_t *create_root(void *obj);
int create_node(vfs_node_t *node, char *path, mode_t mode, struct vfs_file_operations *file_ops, void *obj, vfs_node_t **out);
static vfs_node_t* lookup(vfs_node_t *node, char *name);

vfs_fs_operations_t tempfs_ops = {
    .read_super = &read_super
};

struct vfs_super_operations tempfs_super_ops = {
    .spawn = &spawn_node
};

vfs_node_operations_t tempfs_node_ops = {
    .create_node = &create_node,
    .lookup = &lookup,
};

struct vfs_file_operations tempfs_file_ops = {0};

struct vfs_node *spawn_node(struct vfs_super *super, char *name, mode_t mode)
{
    struct vfs_node *node = kmalloc(sizeof(struct vfs_node));
    memset(node, 0, sizeof(struct vfs_node));
    node->super = super;
    node->mode = mode;
    node->file_ops = &tempfs_file_ops;
    node->ops = &tempfs_node_ops;
    strcpy(node->name, name);
    return node;
}

static struct vfs_super* read_super(struct ata_device *dev)
{
    struct vfs_super *super = kmalloc(sizeof(struct vfs_super));
    memset(super, 0, sizeof(struct vfs_super));
    super->dev = dev;
    super->ops = &tempfs_super_ops;
    return super;
}

void init_tempfs()
{
    struct vfs_fs_type *fs = kmalloc(sizeof(struct vfs_fs_type));
    memset(fs, 0, sizeof(struct vfs_fs_type));
    strcpy(fs->name, "tempfs");
    fs->ops = &tempfs_ops;
    register_fs(fs);
}

static vfs_node_t* lookup(vfs_node_t *node, char *name)
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

/*
 * path - must be a copy
 */
int create_node(vfs_node_t *node, char *path, uint32_t mode, struct vfs_file_operations *file_ops, void *obj, vfs_node_t **out)
{
    vfs_node_t *new = kmalloc(sizeof(vfs_node_t));
    memset(new, 0, sizeof(vfs_node_t));
    new->mode = mode;
    new->obj = obj;
    new->file_ops = (file_ops == NULL ? &tempfs_file_ops : file_ops);
    new->ops = node->ops;
    strcpy(new->name, path);
    add_to_list(node->children, new);
    if (out != 0)
    {
        *out = new;
    }
    return 0;
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
