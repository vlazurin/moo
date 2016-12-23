#include "vfs.h"
#include "debug.h"
#include "string.h"
#include "ring.h"
#include "liballoc.h"
#include "tasking.h"
#include "mutex.h"
#include "libc.h"

#define MAX_FS_TYPES_COUNT 10
#define MAX_PATH_DEPTH 512
#define MAX_PATH_LENGTH 2048

vfs_type_t *vfs_types[MAX_FS_TYPES_COUNT];
vfs_node_t *root = 0;
static mutex_t global_mutex = 0;

int canonicalize_path(char *path, char *out, uint32_t size)
{
    char *copy = strdup(path);
    ring_t *ring = create_ring(MAX_PATH_DEPTH);
    int res = 0;

    char *token;
    while((token = strtok_r(copy, "/", &copy))) {
        if (strlen(token) != 0) {
            if (strcmp(token, VFS_CURRENT_DIR) == 0) {
                continue;
            } else if (strcmp(token, VFS_PARENT_DIR) == 0) {
                ring->head_pop(ring);
            } else {
                ring->push(ring, token);
            }
        }
    }

    *out++ = '/';
    uint8_t separate = 0;
    char *part = ring->pop(ring);
    while(part != NULL) {
        if (separate == 1) {
            *out++ = '/';
        }
        strcpy(out, part);
        uint32_t len = strlen(part);

        if (size - len <= 0) {
            res = VFS_BAD_PATH;
            goto cleanup;
        }
        size -= len;
        out += strlen(part);
        separate = 1;
        part = ring->pop(ring);
    }
    *out = '\0';

cleanup:
    if (copy != NULL) kfree(copy);
    while(ring->pop(ring));
    ring->free(ring);

    return res;
}

int register_fs(vfs_type_t *fs)
{
    mutex_lock(&global_mutex);
    int res = VFS_FS_MAX_COUNT_REACHED;

    for(uint8_t i = 0; i < MAX_FS_TYPES_COUNT; i++) {
        if (vfs_types[i] == NULL) {
            vfs_types[i] = fs;
            debug("[vfs] registered new FS: %s\n", fs->name);
            res = VFS_SUCCESS;
            goto cleanup;
        }
    }

cleanup:
    mutex_release(&global_mutex);
    return res;
}

int mount_fs(char *path, char *fs_name)
{
    int res = 0;
    char *canonized = kmalloc(MAX_PATH_LENGTH);
    int err = canonicalize_path(path, canonized, MAX_PATH_LENGTH);

    if (err) {
        debug("[vfs] mount failed\n");
        res = VFS_BAD_PATH;
        goto cleanup;
    }

    mutex_lock(&global_mutex);
    for(uint16_t i = 0; i < MAX_FS_TYPES_COUNT; i++) {
        if (strcmp(vfs_types[i]->name, fs_name) == 0) {
            if (strcmp(canonized, "/") == 0) {
                root = vfs_types[i]->ops->create_root();
                debug("[vfs] root changed\n");
            } else {
            /*    if (root == 0)
                {
                    return VFS_NOT_INITIALIZED;
                }

                vfs_node_t *fs_root = vfs_types[i]->ops->create_root();
                result = root->ops->create_node(root, canonized, VFS_NODE_TYPE_MOUNT, fs_root->ops, fs_root->file_ops, fs_root);
                kfree(canonized);*/
                goto mutex_cleanup;
            }
        }
    }

mutex_cleanup:
    mutex_release(&global_mutex);
cleanup:
    kfree(canonized);
    return res;
}

int mkdir(char *path)
{
    if (root == NULL) {
        return VFS_NOT_INITIALIZED;
    }

    int res = 0;
    char *canonized = kmalloc(MAX_PATH_LENGTH);
    int err = canonicalize_path(path, canonized, MAX_PATH_LENGTH);
    if (err) {
        debug("[vfs] mkdir failed\n");
        res = err;
        goto cleanup;
    }

    mutex_lock(&global_mutex);
    res = root->ops->create_node(root, canonized, S_IFDIR, 0, 0, 0);
    mutex_release(&global_mutex);

cleanup:
    kfree(canonized);
    return res;
}

int create_vfs_node(char *path, mode_t mode, vfs_file_operations_t *file_ops, void *obj, vfs_node_t **out)
{
    if (root == NULL) {
        return VFS_NOT_INITIALIZED;
    }

    int res = 0;
    char *canonized = kmalloc(MAX_PATH_LENGTH);
    int err = canonicalize_path(path, canonized, MAX_PATH_LENGTH);
    if (err) {
        debug("[vfs] mkdir failed\n");
        res = err;
        goto cleanup;
    }

    mutex_lock(&global_mutex);
    res = root->ops->create_node(root, canonized, mode, file_ops, obj, out);
    mutex_release(&global_mutex);

cleanup:
    kfree(canonized);
    return res;
}

int create_vfs_device(char *path, vfs_file_operations_t *file_ops, void *obj)
{
    return create_vfs_node(path, S_IFCHR, file_ops, obj, 0);
}

int exist_vfs_node(char *path)
{
    if (root == NULL) {
        return VFS_NOT_INITIALIZED;
    }

    int res = 0;
    char *canonized = kmalloc(MAX_PATH_LENGTH);
    int err = canonicalize_path(path, canonized, MAX_PATH_LENGTH);
    if (err) {
        debug("[vfs] mkdir failed\n");
        res = err;
        goto cleanup;
    }

    mutex_lock(&global_mutex);
    res = root->ops->exist(root, canonized);
    mutex_release(&global_mutex);

cleanup:
    kfree(canonized);
    return res;
}

int read_file(file_descriptor_t fd, void *buf, uint32_t size)
{
    if (fd >= MAX_OPENED_FILES || fd == -1 || get_curent_proccess()->files[fd] == 0) {
        return VFS_NOT_FOUND;
    }

    vfs_file_t *file = get_curent_proccess()->files[fd];
    if (file->ops == 0 || file->ops->read == 0)
    {
        return VFS_READ_NOT_POSSIBLE;
    }

    return file->ops->read(file, buf, size);
}

uint32_t sys_write(file_descriptor_t fd, void *buf, uint32_t size)
{
    if (fd >= MAX_OPENED_FILES)
    {
        return 0;
    }

    if (get_curent_proccess()->files[fd] == 0)
    {
        return 0;
    }

    vfs_file_t *file = get_curent_proccess()->files[fd];
    if (file->ops == 0 || file->ops->write == 0)
    {
        return 0;
    }

    return file->ops->write(file, buf, size);
}

file_descriptor_t sys_open(char *path)
{
    if (root == 0)
    {
        return -1;
    }

    char *canonized = kmalloc(MAX_PATH_LENGTH);
    uint8_t result = canonicalize_path(path, canonized, MAX_PATH_LENGTH);
    if (result != VFS_SUCCESS)
    {
        kfree(canonized);
        debug("[vfs] sys_open failed, path not found\n");
        return result;
    }

    vfs_node_t *node = root->ops->get_node(root, canonized);
    if (node == 0)
    {
        debug("[vfs] sys_open failed, node not found\n");
        kfree(canonized);
        return -1;
    }

    vfs_file_t *file = kmalloc(sizeof(vfs_file_t));
    memset(file, 0, sizeof(vfs_file_t));
    file->ops = node->file_ops;
    file->node = node;
    file->pid = get_curent_proccess()->id;
    if (file->ops && file->ops->open)
    {
        file->ops->open(file, 0);
    }

    for(uint32_t i = 0; i < MAX_OPENED_FILES; i++)
    {
        if (get_curent_proccess()->files[i] == 0)
        {
            get_curent_proccess()->files[i] = file;
            kfree(canonized);
            return i;
        }
    }

    kfree(file);
    kfree(canonized);
    return -1;
}

int32_t sys_close(file_descriptor_t fd)
{
    if (fd >= MAX_OPENED_FILES)
    {
        return 0;
    }

    if (get_curent_proccess()->files[fd] == 0)
    {
        return 0;
    }

    vfs_file_t *file = get_curent_proccess()->files[fd];
    get_curent_proccess()->files[fd] = 0;
    if (file->ops == 0 || file->ops->close == 0)
    {
        kfree(file);
        return 0;
    }

    uint32_t result = file->ops->close(file);
    kfree(file);
    return result;
}

int stat_fs(char *path, struct stat *buf)
{
    if (root == NULL) {
        return VFS_NOT_INITIALIZED;
    }

    int res = 0;
    char *canonized = kmalloc(MAX_PATH_LENGTH);
    int err = canonicalize_path(path, canonized, MAX_PATH_LENGTH);
    if (err) {
        debug("[vfs] stat failed\n");
        res = err;
        goto cleanup;
    }

    mutex_lock(&global_mutex);
    res = root->ops->stat(root, canonized, buf);
    mutex_release(&global_mutex);

cleanup:
    kfree(canonized);
    return res;
}

int fstat(file_descriptor_t fd, struct stat *buf)
{
    if (fd >= MAX_OPENED_FILES)
    {
        return 0;
    }

    if (get_curent_proccess()->files[fd] == 0)
    {
        return 0;
    }

    //vfs_file_t *file = get_curent_proccess()->files[fd];
    //file->node->stat_node(file->node, )
    return 0;
}

int fcntl(int fd, int cmd, int arg)
{
    if (fd >= MAX_OPENED_FILES) {
        return 0;
    }

    if (get_curent_proccess()->files[fd] == 0) {
        return 0;
    }

    switch (cmd) {
        case F_DUPFD:
            for(uint32_t i = cmd; i < MAX_OPENED_FILES; i++) {
                if (get_curent_proccess()->files[i] == 0) {
                    //TODO: INCREASE REF COUNT
                    get_curent_proccess()->files[i] = get_curent_proccess()->files[fd];
                    return i;
                }
            }
            return -1;
        break;
        case F_DUPFD_CLOEXEC:
            for(uint32_t i = cmd; i < MAX_OPENED_FILES; i++) {
                if (get_curent_proccess()->files[i] == 0) {
                    //TODO: INCREASE REF COUNT
                    get_curent_proccess()->files[i] = get_curent_proccess()->files[fd];
                    get_curent_proccess()->files[i]->flags |= FD_CLOEXEC;
                    return i;
                }
            }
        break;
        case F_GETFD:
            //TODO: INCREASE REF COUNT
            return get_curent_proccess()->files[fd]->flags;
        break;
        case F_SETFD:
            //TODO: INCREASE REF COUNT
            return get_curent_proccess()->files[fd]->flags = arg;
        break;
        default:
            debug("unknown fcntl cmd %i\n", cmd);
            return -1;
        break;
    }

    return -1;
}
