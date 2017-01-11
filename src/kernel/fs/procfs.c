#include "vfs.h"
#include "liballoc.h"
#include "string.h"
#include "debug.h"

//static vfs_node_t *create_root(void *obj);
static vfs_node_t* lookup(vfs_node_t *node, char *path);

vfs_fs_operations_t procfs_ops = {
//    .create_root = &create_root
};

vfs_node_operations_t procfs_root_node_ops = {
    .create_node = 0,
    .lookup = &lookup
};

static vfs_node_t* lookup(vfs_node_t *node, char *path)
{
    debug("get node!!!\n");
    return 0;
}

void init_procfs()
{
    struct vfs_fs_type *fs = kmalloc(sizeof(struct vfs_fs_type));
    memset(fs, 0, sizeof(struct vfs_fs_type));
    strcpy(fs->name, "procfs");
    fs->ops = &procfs_ops;
    register_fs(fs);
}
/*
vfs_node_t *create_root(void *obj)
{
    vfs_node_t *new = kmalloc(sizeof(vfs_node_t));
    memset(new, 0, sizeof(vfs_node_t));
    new->ops = &procfs_root_node_ops;
    new->mode = S_IFDIR;
    strcpy(new->name, "procfs_root");
    return new;
}*/
