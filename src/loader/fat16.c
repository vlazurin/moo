#include "fat16.h"
#include "string.h"
#include "debug.h"
#include "bios.h"
#include "system.h"

fat_entry_t *root_directory = (fat_entry_t*)ROOT_DIRECTORY_ADDR;
uint16_t *fat_table = (void*)FAT_TABLE_ADDR;

void init_fat()
{
    boot_sector_t *boot_sector = (boot_sector_t*)BOOTSECTOR_ADDR;

    // read root directory
    uint16_t root_dir_start = boot_sector->fat_numbers * boot_sector->sectors_per_fat + boot_sector->reserved_sectors;

    read_lba(root_dir_start, ROOT_DIRECTORY_SIZE / boot_sector->bytes_per_sector, root_directory);
    // read FAT
    read_lba(boot_sector->reserved_sectors, boot_sector->sectors_per_fat, fat_table);
}

uint16_t bytes_per_cluster()
{
    boot_sector_t *boot_sector = (boot_sector_t*)BOOTSECTOR_ADDR;
    return boot_sector->sectors_per_cluster * boot_sector->bytes_per_sector;
}

uint16_t read_cluster(uint16_t file_cluster, void *buffer)
{
    boot_sector_t *boot_sector = (boot_sector_t*)BOOTSECTOR_ADDR;

    if (file_cluster == 0x0)
    {
        print("Can't read file: empty cluster");
        hlt();
    }

    if (file_cluster == 0x1)
    {
        print("Can't read file: wrong cluster");
        hlt();
    }

    if (file_cluster >= 0xFFF0 && file_cluster <= 0xFFF6)
    {
        print("Can't read file: reserved cluster");
        hlt();
    }

    if (file_cluster == 0xFFF7)
    {
        print("Can't read file: bad cluster");
        hlt();
    }

    // end of file
    if (file_cluster > 0xFFF8)
    {
        return 0;
    }

    uint16_t root_directory_size = boot_sector->root_entries_count * ROOT_DIRECTORY_ENTRY_SIZE;
    if (root_directory_size % boot_sector->bytes_per_sector > 0)
    {
        root_directory_size = root_directory_size / boot_sector->bytes_per_sector + 1;
    }
    else
    {
        root_directory_size = root_directory_size / boot_sector->bytes_per_sector;
    }

    uint16_t first_data_sector = boot_sector->reserved_sectors + boot_sector->fat_numbers * boot_sector->sectors_per_fat + root_directory_size;
    uint16_t file_sector = ((file_cluster - 2) * boot_sector->sectors_per_cluster) + first_data_sector;
    read_lba(file_sector, boot_sector->sectors_per_cluster, buffer);
    return fat_table[file_cluster];
}

fat_entry_t *get_fat_entry(const char *filename, const char *extension)
{
    uint16_t name_length = strlen(filename);
    uint16_t ext_length = strlen(extension);

    if (name_length > 8 || name_length == 0)
    {
        print("Can't read file: filename isn't valid");
        hlt();
    }

    if (ext_length > 3)
    {
        print("Can't read file: file extension isn't valid");
        hlt();
    }

    char name[8];
    char ext[3];
    // name and ext must be padded by spaces (fat16 specs)
    memset(&name, 32, 8);
    memset(&ext, 32, 3);
    memcpy(&name, filename, name_length);
    memcpy(&ext, extension, ext_length);

    for(uint16_t i = 0; i < ROOT_DIRECTORY_ENTRY_COUNT; i++)
    {
        // end of directory
        if (root_directory[i].filename[0] == 0x0)
        {
            return (fat_entry_t*)0;
        }

        // 0x0F - long file name record, 0x08 - it`s volume id record, 0xe5 - deleted entry
        if (root_directory[i].filename[0] == 0xe5 || root_directory[i].attributes == 0x0F || root_directory[i].attributes == 0x08)
        {
            continue;
        }

        if (strncmp(root_directory[i].filename, name, 8) == 0 && strncmp(root_directory[i].extension, ext, 3) == 0)
        {
            return &root_directory[i];
        }
    }

    return (fat_entry_t*)0;
}
