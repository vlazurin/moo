#include "vfs.h"
#include "liballoc.h"
#include "system.h"
#include "string.h"
#include "ata.h"
#include "pci.h"
#include "debug.h"
#include "fat16fs.h"
#include "errno.h"
#include <stdbool.h>

static struct vfs_super* read_super(struct ata_device *dev);
static struct vfs_node *spawn_node(struct vfs_super *super, char *name, mode_t mode);
static int create_node(vfs_node_t *node, char *path, mode_t mode, struct vfs_file_operations *file_ops, void *obj, vfs_node_t **out);
static uint8_t exist(vfs_node_t *node, char *path);
static vfs_node_t* lookup(vfs_node_t *node, char *name);
static int stat_node(vfs_node_t *node, char *path, struct stat *buf);
static int read(vfs_file_t *file, void *buffer, uint32_t size, uint32_t *pos);

struct fat16_super_private {
    void *bs;
    uint16_t *fat;
};

vfs_fs_operations_t fat16fs_ops = {
    .read_super = &read_super
};

vfs_node_operations_t fat16fs_node_ops = {
    .exist = &exist,
    .create_node = &create_node,
    .lookup = &lookup,
    .stat = &stat_node
};

struct vfs_super_operations fat16fs_super_ops = {
    .spawn = &spawn_node
};

struct vfs_file_operations fat16fs_file_ops = {
    .read = &read
};

static int read(vfs_file_t *file, void *buffer, uint32_t size, uint32_t *pos)
{
    if (*pos > file->node->size) {
        return -EINVAL;
    }
    struct boot_sector *bs = ((struct fat16_super_private*)file->node->super->obj)->bs;
    uint32_t start_cluster = *pos / (SECTOR_SIZE * bs->sectors_per_cluster);
    uint32_t amount = ALIGN(ALIGN(size, SECTOR_SIZE) / SECTOR_SIZE, bs->sectors_per_cluster) / bs->sectors_per_cluster;
    debug("read %i, %i\n", start_cluster, amount);
    uint8_t *tmp_buf = kmalloc(amount * bs->sectors_per_cluster * SECTOR_SIZE);
    uint32_t cluster = (uint32_t)file->node->obj;
    debug("cluster %i\n", cluster);
    uint8_t *cur = tmp_buf;
    uint32_t done = 0;
    while(done < amount) {
        if (cluster == 0x0 || cluster == 0x1 || (cluster >= 0xFFF0 && cluster <= 0xFFF6) || cluster == 0xFFF7)
        {
            debug("wrong FAT16 cluster %h\n", cluster);
            hlt();
        }

        // end of file
        if (cluster > 0xFFF8)
        {
            break;
        }
        uint16_t root_directory_size = bs->root_entries_count * ROOT_DIRECTORY_ENTRY_SIZE;
        if (root_directory_size % bs->bytes_per_sector > 0)
        {
            root_directory_size = root_directory_size / bs->bytes_per_sector + 1;
        }
        else
        {
            root_directory_size = root_directory_size / bs->bytes_per_sector;
        }
        uint16_t first_data_sector = bs->reserved_sectors + bs->fat_numbers * bs->sectors_per_fat + root_directory_size;
        uint16_t file_sector = ((cluster - 2) * bs->sectors_per_cluster) + first_data_sector;
        int err = file->node->super->dev->read(file->node->super->dev, cur, file_sector, bs->sectors_per_cluster);
        debug("read res %i\n", err);
        debug("read sector %i count %i\n", file_sector, bs->sectors_per_cluster);
        cur += bs->sectors_per_cluster * SECTOR_SIZE;
        debug("next: %i\n", ((struct fat16_super_private*)file->node->super->obj)->fat[cluster]);
        cluster = ((struct fat16_super_private*)file->node->super->obj)->fat[cluster];
        done++;
    }
    *pos += size;
    memcpy(buffer, tmp_buf + (*pos % (SECTOR_SIZE * bs->sectors_per_cluster)), size);

    return size;
}

static void fill_node_children(struct vfs_node *node, struct fat_entry *entry)
{
    while(entry->filename[0] != 0x0)
    {
        if (entry->filename[0] == 0xe5 || entry->attributes == 0x0F || entry->attributes == 0x08)
        {
            continue;
        }
        mode_t mode = (entry->attributes & (1 < 4) ? S_IFDIR : S_IFREG);
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
        new->obj = (void*)(uint32_t)entry->start_cluster;
        add_to_list(node->children, new);
        entry++;
    }
}

static int preload_node(struct vfs_node *node)
{
    if (strcmp(node->name, "/") == 0) {
        struct boot_sector *bs = ((struct fat16_super_private*)node->super->obj)->bs;
        uint16_t root_dir_start = bs->fat_numbers * bs->sectors_per_fat + bs->reserved_sectors;
        struct fat_entry *entry = kmalloc(ROOT_DIRECTORY_SIZE);
        int err = node->super->dev->read(node->super->dev, entry, root_dir_start, ROOT_DIRECTORY_SIZE / bs->bytes_per_sector);
        if (err) {
            kfree(entry);
            return err;
        }
        fill_node_children(node, entry);
        kfree(entry);
    }

    node->is_loaded = true;
    return 0;
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

static struct vfs_super* read_super(struct ata_device *dev)
{
    struct vfs_super *super = kmalloc(sizeof(struct vfs_super));
    memset(super, 0, sizeof(struct vfs_super));
    super->dev = dev;
    super->ops = &fat16fs_super_ops;
    void *bs = kmalloc(SECTOR_SIZE);
    int err = dev->read(dev, bs, 0, 1);
    if (err) {
        kfree(super);
        kfree(bs);
        return NULL;
    }
    struct fat16_super_private *private = kmalloc(sizeof(struct fat16_super_private));
    private->bs = bs;
    struct boot_sector *bootsect = (struct boot_sector*)bs;
    private->fat = kmalloc(bootsect->sectors_per_fat * SECTOR_SIZE);
    err = dev->read(dev, private->fat, bootsect->reserved_sectors, bootsect->sectors_per_fat);
    if (err) {
        kfree(super);
        kfree(bs);
        kfree(private->fat);
        kfree(private);
        return NULL;
    }
    super->obj = private;
    return super;
}

void init_fat16fs()
{
    struct vfs_fs_type *fs = kmalloc(sizeof(struct vfs_fs_type));
    memset(fs, 0, sizeof(struct vfs_fs_type));
    strcpy(fs->name, "fat16fs");
    fs->ops = &fat16fs_ops;
    int err = register_fs(fs);
    if (err) {
        debug("Can't register fat16fs in system, error: %i\n", err);
        return;
    }

    pci_device_t *storage = get_pci_device_by_class(PCI_CLASS_MASS_STORAGE, 0);
    while(storage) {
        if (storage->subclass == PCI_SUB_CLASS_IDE) {
            struct ide_controller *ide = storage->hardware_driver;
            for(uint16_t c = 0; c < 2; c++) {
                for(uint16_t d = 0; d < 2; d++) {
                    if (ide->channels[c].devices[d] != NULL) {
                        struct boot_sector *bs = kmalloc(SECTOR_SIZE);
                        int err = ide->channels[c].devices[d]->read(ide->channels[c].devices[d], bs, 0, 1);
                        if (err) {
                            debug("init_fat16fs: ata read failed\n");
                            kfree(bs);
                            continue;
                        }
                        if (strncmp(bs->fat_type, "FAT16", 5) == 0) {
                            char name[30];
                            char volume[12];
                            memcpy(volume, bs->volume_name, 11);
                            volume[11] = '\0';
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

static int stat_node(vfs_node_t *node, char *path, struct stat *buf)
{
    return 0;
}

static uint8_t exist(vfs_node_t *node, char *path)
{
    return 1;
}

static vfs_node_t* lookup(vfs_node_t *node, char *name)
{
    int err = 0;
    if (node->is_loaded == 0) {
        err = preload_node(node);
        if (err) {
            return NULL;
        }
    }
    vfs_node_t *iterator = node->children;
    while(iterator != 0)
    {
        if (strcmp(iterator->name, name) == 0)
        {
            return iterator;
        }
        iterator = (vfs_node_t*)iterator->list.next;
    }
    return NULL;
}

/*
 * path - must be a copy
 */
int create_node(vfs_node_t *node, char *path, uint32_t mode, struct vfs_file_operations *file_ops, void *obj, vfs_node_t **out)
{
    return -EROFS;
}
