#ifndef H_FAT_16
#define H_FAT_16

#include <stdint.h>

#define ROOT_DIRECTORY_ENTRY_COUNT 512
#define ROOT_DIRECTORY_ADDR 0x10000
#define ROOT_DIRECTORY_SIZE 0x4000
#define ROOT_DIRECTORY_ENTRY_SIZE 32
#define FAT_TABLE_ADDR 0x18000
#define BOOTSECTOR_ADDR 0x7c00

typedef struct boot_sector
{
    uint8_t jmp[3];
    uint8_t oem_identifier[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  fat_numbers;
    uint16_t root_entries_count;
    uint16_t sectors_count;
    uint8_t  media_descriptor;
    uint16_t sectors_per_fat;
    uint16_t sectors_per_head;
    uint16_t heads_per_cylinder;
    uint32_t hidden_sectors;
    uint32_t sectors_per_partition;
    uint8_t logical_drive_number;
    uint8_t head_number;
    uint8_t boot_signature;
    uint32_t device_serial;
    uint8_t volume_name[11];
    uint8_t fat_type[8];

} __attribute__((packed)) boot_sector_t;

typedef struct fat_entry
{
    char filename[8];
    char extension[3];
    uint8_t attributes;
    uint8_t reserved;
    uint8_t creation;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access_date;
    uint16_t reserved_fat32;
    uint16_t last_write_time;
    uint16_t last_write_date;
    uint16_t start_cluster;
    uint32_t file_size;

} __attribute__((packed)) fat_entry_t;

void init_fat();
uint16_t read_cluster(uint16_t file_cluster, void *buffer);
fat_entry_t *get_fat_entry(const char *filename, const char *extension);
uint16_t bytes_per_cluster();

#endif
