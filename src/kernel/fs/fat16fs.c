#include "vfs.h"
#include "liballoc.h"
#include "task.h"
#include "system.h"
#include "string.h"
#include "ata.h"
#include "pci.h"
#include "libc.h"
#include "log.h"
#include "fat16fs.h"
#include "errno.h"
#include <stdbool.h>

static struct vfs_super* read_super(struct ata_device *dev);
static struct vfs_node *spawn_node(struct vfs_super *super, char *name, mode_t mode);
static int create_node(vfs_node_t *node, char *path, mode_t mode, struct vfs_file_operations *file_ops, void *obj, vfs_node_t **out);
static vfs_node_t* lookup(vfs_node_t *node, char *name);
static int read(vfs_file_t *file, void *buffer, uint32_t size, uint32_t *pos);
static int write(vfs_file_t *file, void *buffer, uint32_t size, uint32_t *pos);

struct fat16_super_private {
    struct boot_sector bs;
    uint16_t *fat;
    uint16_t clusters_count;
    uint16_t bytes_per_cluster;
    uint16_t root_dir_sector;
};

struct fat16_node_private {
    uint16_t cluster;
    uint32_t directory_index;
};

static vfs_fs_operations_t fat16fs_ops = {
    .read_super = &read_super
};

static vfs_node_operations_t fat16fs_node_ops = {
    .create_node = &create_node,
    .lookup = &lookup,
};

static struct vfs_super_operations fat16fs_super_ops = {
    .spawn = &spawn_node
};

static struct vfs_file_operations fat16fs_file_ops = {
    .read = &read,
    .write = &write
};

/**
 * Internal use only.
 * Converts cluster number to LBA address
 */
static uint16_t cluster_to_sector(struct boot_sector *bs, uint16_t cluster)
{
    assert((intptr_t)bs >= KERNEL_SPACE_ADDR);
    assert(cluster > 1);

    uint16_t first_data_sector = bs->reserved_sectors + bs->fat_numbers * bs->sectors_per_fat + FAT16_ROOT_DIRECTORY_SECTORS;
    uint16_t sector = ((cluster - 2) * bs->sectors_per_cluster) + first_data_sector;
    return sector;
}

/**
 * Internal use only.
 * Reads one cluster from disk.
 */
static int read_cluster(struct vfs_super *super, uint16_t cluster, void *buffer)
{
    assert((intptr_t)super >= KERNEL_SPACE_ADDR);
    assert((intptr_t)super->private >= KERNEL_SPACE_ADDR);
    assert((intptr_t)buffer >= KERNEL_SPACE_ADDR);

    if (cluster <= 0x1 || (cluster >= 0xFFF0 && cluster <= 0xFFF7)) {
        log(KERN_ERR, "detected Illegal FAT16 cluster (%i)\n", cluster);
        return -EIO;
    }

    if (cluster > 0xFFF7) {
        log(KERN_ERR, "can't read free FAT16 cluster (%i)\n", cluster);
        return -EIO;
    }

    struct fat16_super_private *private = super->private;
    uint16_t sector = cluster_to_sector(&private->bs, cluster);
    int err = super->dev->read(super->dev, buffer, sector, private->bs.sectors_per_cluster);
    return err;
}

/**
 * Internal use only.
 * Reads clusters chain from disk.
 */
static int read_chain(struct vfs_super *super, uint16_t cluster, void *buffer)
{
    assert((intptr_t)super >= KERNEL_SPACE_ADDR);
    assert((intptr_t)super->private >= KERNEL_SPACE_ADDR);
    assert((intptr_t)buffer >= KERNEL_SPACE_ADDR);

    struct fat16_super_private *private = super->private;
    while(cluster > 0x1 && cluster < 0xFFF7) {
        int err = read_cluster(super, cluster, buffer);
        debug("read chain cluster %i, err %i, sectors_per_cluster %i\n", cluster, err, private->bs.sectors_per_cluster);
        if (err) {
            return err;
        }
        buffer += private->bytes_per_cluster;
        cluster = private->fat[cluster];
    }

    return 0;
}

static int count_clusters_in_chain(struct fat16_super_private *private, uint16_t cluster)
{
    int count = 0;
    while(cluster > 0x1 && cluster < 0xFFF7) {
        count++;
        cluster = private->fat[cluster];
    }
    return count;
}

static void process_directory_entries(struct vfs_node *node, struct fat_entry *entry, int count)
{
    int index = 0;
    for(;count > 0; count--)
    {
        if (entry->filename[0] == 0x0 || entry->filename[0] == 0xe5 || entry->attributes == 0x0F || entry->attributes == 0x08)
        {
            entry++;
            index++;
            continue;
        }

        mode_t mode = ((entry->attributes & 0x10) ? S_IFDIR : S_IFREG);
        char name[13] = {0};
        memcpy(name, entry->filename, 8);
        char *ptr = trim(name);
        ptr += strlen(ptr);
        if (entry->extension[0] != ' ') {
            *ptr++ = '.';
        }
        memcpy(ptr, entry->extension, 3);
        strlower(name, 12);
        ptr = trim(name);
        struct vfs_node *new = node->super->ops->spawn(node->super, ptr, mode);
        new->size = entry->file_size;
        struct fat16_node_private *node_p = kmalloc(sizeof(struct fat16_node_private));
        node_p->cluster = (uint32_t)entry->start_cluster;
        node_p->directory_index = index;
        new->obj = node_p;
        new->parent = node;
        debug("%s %i\n", new->name, entry->start_cluster);
        add_to_list(node->children, new);
        entry++;
        index++;
    }
}

static int load_directory_content(struct vfs_node *node)
{
    assert(node != NULL);

    struct fat16_super_private *private = node->super->private;
    struct ata_device *dev = node->super->dev;
    struct fat_entry *entries = NULL;
    int err = 0;

    if (node == node->super->local_root) {
        entries = kmalloc(FAT16_ROOT_DIRECTORY_SIZE);
        err = dev->read(dev, entries, private->root_dir_sector, FAT16_ROOT_DIRECTORY_SECTORS);
        if (err) {
            goto cleanup;
        }
        process_directory_entries(node, entries, FAT16_DIRECTORY_ENTRY_COUNT);
    } else {
        struct fat16_node_private *node_private = node->obj;
        uint16_t cluster = node_private->cluster;
        uint32_t alloc = count_clusters_in_chain(private, cluster) * private->bytes_per_cluster;
        assert(alloc > 0);
        entries = kmalloc(alloc);
        err = read_chain(node->super, cluster, entries);
        if (err) {
            goto cleanup;
        }
        process_directory_entries(node, entries, alloc / sizeof(struct fat_entry));
    }

    node->loaded = true;
cleanup:
    kfree(entries);
    return err;
}

static vfs_node_t* lookup(vfs_node_t *node, char *name)
{
    assert(name != NULL);
    if (node->loaded == false) {
        assert(S_ISDIR(node->mode));
        int err = load_directory_content(node);
        if (err) {
            return NULL;
        }
    }

    vfs_node_t *iterator = node->children;
    while(iterator != NULL) {
        if (strcmp(iterator->name, name) == 0) {
            return iterator;
        }
        iterator = (vfs_node_t*)iterator->list.next;
    }
    return NULL;
}

static struct vfs_super* read_super(struct ata_device *dev)
{
    struct fat16_super_private *private = NULL;

    struct vfs_super *super = kmalloc(sizeof(struct vfs_super));
    memset(super, 0, sizeof(struct vfs_super));
    super->dev = dev;
    super->ops = &fat16fs_super_ops;

    struct boot_sector *bs = kmalloc(SECTOR_SIZE);
    int err = dev->read(dev, bs, 0, 1);
    if (err) {
        goto cleanup;
    }

    uint32_t fat_size = bs->sectors_per_fat * SECTOR_SIZE;
    private = kmalloc(sizeof(struct fat16_super_private));
    private->bs = *bs;
    private->clusters_count = fat_size / FAT16_BYTES_PER_FAT_ENTRY;
    private->fat = kmalloc(fat_size);
    private->bytes_per_cluster = bs->bytes_per_sector * bs->sectors_per_cluster;
    private->root_dir_sector = bs->fat_numbers * bs->sectors_per_fat + bs->reserved_sectors;
    err = dev->read(dev, private->fat, private->bs.reserved_sectors, private->bs.sectors_per_fat);
    if (err) {
        goto cleanup;
    }
    super->private = private;

cleanup:
    kfree(bs);
    if (err) {
        log(KERN_ERR, "can't load super for fat16 disk\n");
        kfree(super);
        super = NULL;
        if (private != NULL) {
            if (private->fat != NULL) kfree(private->fat);
            kfree(private);
        }
    }
    return super;
}

static struct vfs_node *spawn_node(struct vfs_super *super, char *name, mode_t mode)
{
    struct vfs_node *node = kmalloc(sizeof(struct vfs_node));
    memset(node, 0, sizeof(struct vfs_node));
    node->super = super;
    node->mode = mode;
    node->file_ops = &fat16fs_file_ops;
    node->ops = &fat16fs_node_ops;
    strcpy(node->name, name);
    return node;
}

static int read(vfs_file_t *file, void *buffer, uint32_t size, uint32_t *pos)
{
    if (*pos >= file->node->size) {
        return 0;
    }

    struct fat16_super_private *private = file->node->super->private;

    uint32_t bytes_in_cluster = SECTOR_SIZE * private->bs.sectors_per_cluster;
    uint32_t start_from_cluster = *pos / bytes_in_cluster;
    void *read_buffer = kmalloc(bytes_in_cluster);

    uint32_t readed = 0;
    uint16_t skipped_clusters = 0;
    uint32_t pos_in_cluster = *pos % bytes_in_cluster;
    uint16_t cluster = ((struct fat16_node_private*)file->node->obj)->cluster;
    debug("reading %i %i\n", cluster, *pos);
    while(readed < size && cluster <= 0xFFF8) {
        if (skipped_clusters < start_from_cluster) {
            cluster = private->fat[cluster];
            skipped_clusters++;
            continue;
        }
        int err = read_cluster(file->node->super, cluster, read_buffer);
        if (err) {
            kfree(read_buffer);
            return err;
        }

        uint32_t copy = MIN(size - readed, MIN(file->node->size, bytes_in_cluster - pos_in_cluster));
        //debug("fat 16 read: copy to output buffer: shifts are %i %i, amount %i\n", readed, pos_in_cluster, copy);
        memcpy(buffer + readed, read_buffer + pos_in_cluster, copy);
        pos_in_cluster = 0;
        readed += copy;
        cluster = private->fat[cluster];
    }
    *pos += readed;
    kfree(read_buffer);
    return readed;
}

/*static struct fat_entry *get_free_entry(void *dir)
{
    assert((intptr_t)dir >= KERNEL_SPACE_ADDR);

    struct fat_entry *iterator = dir;
    for(int i = 0; i < FAT16_DIRECTORY_ENTRY_COUNT; i++) {
        if (iterator->filename[0] == 0x0) {
            return iterator;
        }
        iterator++;
    }

    return NULL;
}*/

static int get_free_cluster(struct fat16_super_private *private)
{
    for (int i = 0; i < private->clusters_count; i++) {
        if (private->fat[i] == 0x0) {
            return i;
        }
    }

    return -ENOSPC;
}

static int get_cluster_in_chain(struct fat16_super_private *private, struct vfs_node *node, int num)
{
    assert(node != NULL && "probable write attemp is done for root entry");
    struct fat16_node_private *node_private = node->obj;
    uint16_t cluster = node_private->cluster;
    while (num > 0) {
        cluster = private->fat[cluster];
        num--;
    }
    if (num > 0) {
        return -EIO;
    }
    return cluster;
}

static int resize_chain(struct vfs_node *node, int count)
{
    assert(count > 0);
    if (count == 0) {
        return 0;
    }

    struct fat16_super_private *private = node->super->private;
    struct fat16_node_private *node_private = node->obj;
    int cluster = node_private->cluster;

    if (node_private->cluster == 0x0) {
        int next = get_free_cluster(private);
        debug("added cluster %i to directory entry\n", next);
        if (next < 0) {
            return next;
        }
        node_private->cluster = next;
        private->fat[next] = 0xFFFF;
        cluster = next;
        count--;
    }

    while(count > 0) {
        count--;
        if (private->fat[cluster] > 0x1 && private->fat[cluster] < 0xFFF7) {
            cluster = private->fat[cluster];
            continue;
        }
        int next = get_free_cluster(private);
        debug("added cluster %i to fat\n", next);
        if (next < 0) {
            return next;
        }
        private->fat[cluster] = next;
        private->fat[next] = 0xFFFF;
        cluster = next;
    }
    return 0;
}

static int write_cluster(struct vfs_super *super, uint16_t cluster, void *buffer)
{
    assert((intptr_t)super >= KERNEL_SPACE_ADDR);
    assert((intptr_t)super->private >= KERNEL_SPACE_ADDR);
    assert((intptr_t)buffer >= KERNEL_SPACE_ADDR);

    if (cluster <= 0x1 || (cluster >= 0xFFF0 && cluster <= 0xFFF7)) {
        log(KERN_ERR, "detected Illegal FAT16 cluster (%i)\n", cluster);
        return -EIO;
    }

    if (cluster > 0xFFF7) {
        log(KERN_ERR, "can't write free FAT16 cluster (%i)\n", cluster);
        return -EIO;
    }

    struct fat16_super_private *private = super->private;
    uint16_t sector = cluster_to_sector(&private->bs, cluster);
    int err = super->dev->write(super->dev, buffer, sector, private->bs.sectors_per_cluster);
    return err;
}

static int write(vfs_file_t *file, void *buffer, uint32_t size, uint32_t *pos)
{
    // 1 - calc number of required clusters and expand chain
    // 2 - write data to clusters
    // 3 - load directory entries and update file size
    // 4 - flush fat and entry changes to disk

    struct vfs_node *node = file->node;
    struct ata_device *dev = file->node->super->dev;
    struct fat16_super_private *private = node->super->private;
    struct fat16_node_private *node_private = file->node->obj;

    ////////////////////////
    uint16_t required = ALIGN(*pos + size, private->bytes_per_cluster) / private->bytes_per_cluster;
    int err = resize_chain(file->node, required);
    if (err) {
        // restore FAT state
        int err_read = dev->read(dev, private->fat, private->bs.reserved_sectors, private->bs.sectors_per_fat);
        if (err_read) {
            log(KERN_FATAL, "fat16 write failed, system is unstable to reload FAT\n");
            hlt();
        }
        return err;
    }
    ////////////////////////
    int offset = *pos % private->bytes_per_cluster;
    int amount = MIN(private->bytes_per_cluster - offset, size);
    void *cluster_buffer = kmalloc(private->bytes_per_cluster);
    uint16_t cluster = get_cluster_in_chain(private, node, *pos / private->bytes_per_cluster);
    uint32_t left = size;
//    debug("need to write %i bytes from %i pos\n", left, *pos);
//    debug("start cluster is %i\n", cluster);
    while(left > 0) {
        if ((cluster > 0x1 && cluster < 0xFFF7) == false) {
            log(KERN_FATAL, "fat16 write failed, fat chain is broken\n");
            hlt();
        }

        err = read_cluster(node->super, cluster, cluster_buffer);
        if (err) {
            log(KERN_ERR, "fat16 read failed\n");
            kfree(cluster_buffer);
            return err;
        }

        //debug("writing %i bytes from %i offset in cluster %i\n", size, offset, cluster);
        memcpy(cluster_buffer + offset, buffer, amount);
        write_cluster(node->super, cluster, cluster_buffer);
        amount = MIN(private->bytes_per_cluster, left);
        left -= amount;
        offset = 0;
        cluster = private->fat[cluster];
    }
    ////////////////////////
    if (*pos + size > node->size) {
        node->size = *pos + size;
    }

    int entry_cluster_number = node_private->directory_index * sizeof(struct fat_entry) / private->bytes_per_cluster;
    cluster = get_cluster_in_chain(private, node->parent, entry_cluster_number);
    err = read_cluster(node->super, cluster, cluster_buffer);
    if (err) {
        log(KERN_ERR, "fat16 read entry failed\n");
        kfree(cluster_buffer);
        return err;
    }
    offset = node_private->directory_index * sizeof(struct fat_entry) % private->bytes_per_cluster;
    struct fat_entry *entry = cluster_buffer + offset;
//    debug("name is %s, index %i %i %i %i\n", entry->filename, index, node_private->directory_index, entry_cluster_number, cluster);
    entry->start_cluster = node_private->cluster;
    entry->file_size = node->size;
    err = write_cluster(node->super, cluster, cluster_buffer);
    if (err) {
        log(KERN_FATAL, "fat16 write entry failed\n");
        hlt();
    }
    err = dev->write(dev, private->fat, private->bs.reserved_sectors, private->bs.sectors_per_fat);
    if (err) {
        log(KERN_FATAL, "fat16 write fat failed\n");
        hlt();
    }

    kfree(cluster_buffer);
    return size;
}

void init_fat16fs()
{
    struct vfs_fs_type *fs = kmalloc(sizeof(struct vfs_fs_type));
    memset(fs, 0, sizeof(struct vfs_fs_type));
    strcpy(fs->name, "fat16fs");
    fs->ops = &fat16fs_ops;
    int err = register_fs(fs);
    if (err) {
        log(KERN_ERR, "fat16 fs registration failed, errno %i\n", err);
        return;
    }

    pci_device_t *storage = get_pci_device_by_class(PCI_CLASS_MASS_STORAGE, NULL);
    while(storage) {
        if (storage->subclass == PCI_SUB_CLASS_IDE) {
            struct ide_controller *ide = storage->hardware_driver;
            for(uint16_t c = 0; c < 2; c++) {
                for(uint16_t d = 0; d < 2; d++) {
                    if (ide->channels[c].devices[d] != NULL) {
                        struct boot_sector *bs = kmalloc(SECTOR_SIZE);
                        int err = ide->channels[c].devices[d]->read(ide->channels[c].devices[d], bs, 0, 1);
                        if (err) {
                            log(KERN_ERR, "fat16 init failed, can't read from ata device\n");
                            kfree(bs);
                            continue;
                        }
                        if (strncmp(bs->fat_type, "FAT16", 5) == 0) {
                            char name[30] = {0};
                            char volume[12] = {0};
                            memcpy(volume, bs->volume_name, 11);
                            sprintf(name, "/mount/%s", trim(volume));

                            mount_fs(name, "fat16fs", ide->channels[c].devices[d]);
                        }
                        kfree(bs);
                    }
                }
            }
        }
        storage = get_pci_device_by_class(PCI_CLASS_MASS_STORAGE, storage);
    }
}

static int fill_new_entry(struct fat_entry *entry, struct vfs_node *node)
{
    memset(entry->filename, ' ', 8);
    memset(entry->extension, ' ', 3);
    // split name and extension
    char name[12] = {0};
    memcpy(&name, node->name, strlen(node->name));
    int pos = strpos(name, '.');
    char *ext = NULL;
    int len = 0;
    if (pos >= 0) {
        ext = name + pos;
        *ext++ = '\0';
        len = strlen(ext);
        if (len > 3) {
            return -EINVAL;
        }
        strup(ext, len);
        memcpy(entry->extension, ext, len);
    }
    len = strlen(name);
    if (len > 8 || len == 0) {
        return -EINVAL;
    }
    strup(name, len);
    memcpy(entry->filename, name, len);

    entry->attributes = S_ISDIR(node->mode) ? 0x10 : 0;
    entry->start_cluster = 0x0;

    return 0;
}

static int reset_entries_in_cluster(struct vfs_super *super, uint16_t cluster)
{
    struct fat16_super_private *private = super->private;
    int count = private->bytes_per_cluster / sizeof(struct fat_entry);

    void *buffer = kmalloc(private->bytes_per_cluster);
    int err = read_cluster(super, cluster, buffer);
    if (err) {
        kfree(buffer);
        return err;
    }

    struct fat_entry *entry = buffer;
    for (int i = 0; i < count; i++) {
        entry->filename[0] = 0x0;
        entry++;
    }

    err = write_cluster(super, cluster, buffer);
    kfree(buffer);
    if (err) {
        return err;
    }

    return 0;
}

/*
 * path - must be a copy
 */
static int create_node(vfs_node_t *node, char *path, uint32_t mode, struct vfs_file_operations *file_ops, void *obj, vfs_node_t **out)
{
    // 1 - load directory entries
    // 2 - found free entry or add new cluster to chain
    // 3 - fill entry data
    // 4 - flush fat and directory entries changes to disk
    if (S_ISREG(mode) == false && S_ISDIR(mode) == false) {
        return -EINVAL;
    }
    if (strlen(path) > 12) {
        return -E2BIG;
    }

    struct fat16_super_private *private = node->super->private;
    struct fat16_node_private *node_private = node->obj;

    // create VFS node
    struct vfs_node *new = node->super->ops->spawn(node->super, path, mode);

    int node_chain_size = count_clusters_in_chain(private, node_private->cluster);
    void *buffer = kmalloc(private->bytes_per_cluster);
    struct fat_entry *entry = NULL;
    struct fat16_node_private *new_private = NULL;
    int cluster = node_private->cluster;
    int entry_filled = false;
    int err = 0;
    debug("node is %s, start cluster is %i\n", node->name, node_private->cluster);
    for (int i = 0; i < node_chain_size; i++) {
        err = read_cluster(node->super, cluster, buffer);
        if (err) {
            goto cleanup;
        }
        entry = buffer;
        int limit = private->bytes_per_cluster / sizeof(struct fat_entry);

        for(int x = 0; x < limit; x++) {
            if (entry->filename[0] == 0x0) {
                entry_filled = true;
                err = fill_new_entry(entry, new);
                if (err) {
                    goto cleanup;
                }

                int first_cluster = 0;
                if (S_ISDIR(mode)) {
                    first_cluster = get_free_cluster(private);
                    debug("new node is dir, allocating new cluster (%i) for entries\n", first_cluster);
                    err = reset_entries_in_cluster(node->super, first_cluster);
                    if (err) {
                        goto cleanup;
                    }
                    private->fat[first_cluster] = 0xFFFF;
                    entry->start_cluster = first_cluster;

                    debug("dir cluster is clean and ready\n");
                }

                new_private = kmalloc(sizeof(struct fat16_node_private));
                new_private->cluster = first_cluster;
                new_private->directory_index = x;

                break;
            }
            entry++;
        }

        if (entry_filled) {
            break;
        }
        cluster = private->fat[cluster];
    }

    if (entry_filled == false) {
        log(KERN_FATAL, "fa16 no free entries left. implement resize\n");
        hlt();

    }
    err = write_cluster(node->super, cluster, buffer);
    if (err) {
        goto cleanup;
    }

    if (S_ISDIR(mode)) {
        debug("flushing fat changes to disk\n");
        err = node->super->dev->write(node->super->dev, private->fat, private->bs.reserved_sectors, private->bs.sectors_per_fat);
        if (err) {
            log(KERN_FATAL, "fat16 write fat failed\n");
            hlt();
        }
    }

    new->obj = new_private;
    add_to_list(node->children, new);
    if (out != NULL) {
        *out = new;
    }

cleanup:
    if (err) kfree(new);
    if (err && new_private != NULL) kfree(new_private);
    kfree(entry);
    return err;
}
