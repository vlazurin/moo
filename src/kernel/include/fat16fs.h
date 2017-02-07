#ifndef H_FAT16FS
#define H_FAT16FS

#define ROOT_DIRECTORY_ENTRY_COUNT 512
#define FAT16_ENTRY_SIZE 32
#define ROOT_DIRECTORY_SIZE 0x4000

struct boot_sector
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
    char fat_type[8];

} __attribute__((packed));

struct fat_entry
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

} __attribute__((packed));

void init_fat16fs();

#endif
