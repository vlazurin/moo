#include "bios.h"
#include "debug.h"
#include "system.h"
#include "string.h"

// very simple and limited print function. Every call prints string on new line.
void print(const char *str)
{
    static uint8_t cy = 0;

    if (cy == 25)
    {
        cy = 24;
        // scroll one line up
        asm("pusha");
        asm("mov $0x01, %al");
        asm("xor %bh, %bh");
        asm("xor %cx, %cx");
        asm("mov $25, %dh");
        asm("mov $80, %dl");
        asm("mov $0x06, %ah");
        asm("int $0x10");
        asm("popa");
    }

    uint16_t size = strlen(str);
    asm("pusha");
    // must be before BP register change, because gcc uses BP for mov operation
    asm("movw %0, %%cx" :: "r"(size));
    asm("movb %0, %%dh" :: "r"(cy));

    asm("movw %0, %%bp" :: "m"(str));
    asm("mov $0x13, %ah");
    asm("mov $0x01, %al");
    asm("xor %bh,%bh");
    // color
    asm("mov $0x7, %bl");
    // x position
    asm("mov $0, %dl");
    asm("int $0x10");
    asm("popa");

    cy++;
}

uint8_t detect_upper_memory(memory_map_entry_t* memory_map)
{
    uint32_t ebx = 0;
    uint8_t cl = 0;
    uint8_t counter = 0;

    do
    {
        // failsafe for case when BIOS returned 20 bytes only
        memory_map->ACPI = 0;

        uint16_t offset = OFFSET((uintptr_t)memory_map);
        uint16_t segment = SEG((uintptr_t)memory_map);

        save_regs();
        asm("movw %0, %%di" :: "r"(offset));
        asm("movw %0, %%es" :: "r"(segment));
        asm("movl %0, %%ebx" :: "r"(ebx));

        // extended asm instructions must be first, they will use general purpose regs
        // magic number
        asm("mov $0x534D4150, %edx");
        asm("mov $0xE820, %eax");
        asm("mov $0x18, %ecx");
        asm("int $0x15");

        asm("movl %%ebx, %0" : "=r"(ebx));
        asm("movb %%cl, %0" : "=r"(cl));
        restore_regs();

        // soft limit
        if (counter == 100)
        {
            print("[OS Loader] Can't detect upper memory. Memory map is to big");
            hlt();
        }
        memory_map++;
        counter++;
    } while(ebx > 0);

    return counter;
}

void set_video_mode(video_settings_t *video_settings)
{
    // bad idea to store vbe_info struct in stack because of struct size
    vbe_info_t info;
    memcpy(&info.signature, "VBE2", 4);

    uint16_t offset = OFFSET((uintptr_t)&info);
    uint16_t segment = SEG((uintptr_t)&info);
    uint16_t ax = 0;

    save_regs();
    // extended asm instructions must be first, they will use general purpose regs
    asm("movw %0, %%di" :: "r"(offset));
    asm("movw %0, %%es" :: "r"(segment));
    asm("mov $0x4F00, %ax");
    asm("int $0x10");
    asm("movw %%ax, %0" : "=r"(ax));

    restore_regs();

    if (ax != 0x004F)
    {
        print("Video card doen't support VESA");
        hlt();
    }

    uint16_t *modes = (uint16_t*)info.video_modes;
    vbe_mode_info_t mode_info;
    segment = SEG((uintptr_t)&mode_info);
    offset = OFFSET((uintptr_t)&mode_info);
    while (*modes != 0xFFFF)
    {
        save_regs();
        // extended asm instructions must be first, they will use general purpose regs
        asm("movw %0, %%di" :: "r"(offset));
        asm("movw %0, %%es" :: "r"(segment));
        asm("movw %0, %%cx" :: "r"(*modes));
        asm("mov $0x4F01, %ax");
        asm("int $0x10");
        asm("movw %%ax, %0" : "=r"(ax));
        restore_regs();

        if (ax != 0x004F)
        {
            print("Can't get VESA video mode info");
            hlt();
        }
        if (mode_info.bpp == 32 && mode_info.attributes & (1 << 7) && mode_info.width == 640 && mode_info.height == 480)
        {
            uint16_t mode = *modes;
            mode |= (1 << 14); // enable LFB
            mode |= (0 << 15); // clear screen
            save_regs();
            // extended asm instructions must be first, they will use general purpose regs
            asm("movw %0, %%bx" :: "r"(mode));
            asm("mov $0x4F02, %ax");
            asm("int $0x10");
            asm("movw %%ax, %0" : "=r"(ax));
            restore_regs();

            if (ax != 0x004F)
            {
                print("Can't set VESA video mode");
                hlt();
            }

            video_settings->framebuffer = mode_info.framebuffer;
            video_settings->width = mode_info.width;
            video_settings->height = mode_info.height;
            return;
        }

        modes++;
    }

    return;
}
