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

    uint16_t offset = 0;
    uint16_t segment = 0;

    do
    {
        // failsafe for case when BIOS returned 20 bytes only
        memory_map->ACPI = 0;

        if ((uintptr_t)memory_map >= 0x10000)
        {
            segment = (uintptr_t)memory_map / 0x10000 * 0x1000;
            offset = (uintptr_t)memory_map % 0x10000;
        }
        else
        {
            offset = (uintptr_t)memory_map;
        }

        asm("pusha");
        asm("movw %es, %ax");
        asm("push %ax");
        asm("movw %0, %%di" :: "r"(offset));
        asm("movw %0, %%es" :: "r"(segment));
        asm("movl %0, %%ebx" :: "r"(ebx));

        // magic number
        asm("mov $0x534D4150, %edx");
        asm("mov $0xE820, %eax");
        asm("mov $0x18, %ecx");
        asm("int $0x15");

        asm ("movl %%ebx, %0" : "=r"(ebx));
        asm ("movb %%cl, %0" : "=r"(cl));
        asm("pop %ax");
        asm("mov %ax, %es");
        asm("popa");

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
