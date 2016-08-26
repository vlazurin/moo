#ifndef H_BIOS
#define H_BIOS

#include <stdint.h>
#include "system.h"

#define save_regs() asm("pusha; movw %es, %ax; push %ax;")
#define restore_regs() asm("pop %ax; mov %ax, %es; popa;")
#define SEG(value) ((value / 0x10000) * 0x1000)
#define OFFSET(value) (value % 0x10000)

typedef struct lba_disk_address_packet
{
    uint8_t size;
    uint8_t reserved;
    uint16_t sectors_count;
    uint16_t buffer_offset;
    uint16_t buffer_segment;
    uint32_t lba_address;
    uint32_t lba_address_upper;
} __attribute__((packed, aligned (4))) lba_disk_address_packet_t;

typedef struct vbe_info
{
   uint8_t signature[4];
   uint16_t version;
   uint32_t oem;
   uint32_t capabilities;
   uint32_t video_modes;
   uint16_t video_memory;
   uint16_t software_rev;
   uint32_t vendor;
   uint32_t product_name;
   uint32_t product_rev;
   char reserved[222];
   char oem_data[256];
} __attribute__ ((packed)) vbe_info_t;

typedef struct vbe_mode_info
{
   uint16_t attributes;
   uint8_t window_a;
   uint8_t window_b;
   uint16_t granularity;
   uint16_t window_size;
   uint16_t segment_a;
   uint16_t segment_b;
   uint32_t win_func_ptr;
   uint16_t pitch;
   uint16_t width;
   uint16_t height;
   uint8_t w_char;
   uint8_t y_char;
   uint8_t planes;
   uint8_t bpp;
   uint8_t banks;
   uint8_t memory_model;
   uint8_t bank_size;
   uint8_t image_pages;
   uint8_t reserved_0;

   uint8_t red_mask;
   uint8_t red_position;
   uint8_t green_mask;
   uint8_t green_position;
   uint8_t blue_mask;
   uint8_t blue_position;
   uint8_t reserved_mask;
   uint8_t reserved_position;
   uint8_t direct_color_attributes;

   uint32_t framebuffer;
   uint32_t off_screen_mem_off;
   uint16_t off_screen_mem_size;
   uint8_t reserved_1[206];
} __attribute__ ((packed)) vbe_mode_info_t;

void read_lba(uint32_t lba_address, uint16_t sectors_count, void *buffer);
void print(const char *str);
uint8_t detect_upper_memory(memory_map_entry_t *memory_map);
void set_video_mode(video_settings_t *video_settings);

#endif
