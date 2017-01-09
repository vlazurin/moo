#ifndef H_IRQ
#define H_IRQ

#include <stdint.h>

struct idt_entry {
    uint16_t base_low;
    uint16_t selector;
    uint8_t reserved;
    uint8_t flags;
    uint16_t base_high;
} __attribute__((packed));

struct idt {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

// Order of registers is important
struct regs {
    uint32_t ds;
    uint32_t es;
    uint32_t fs;
    uint32_t gs;

    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp; // popa ignores it
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;

    uint32_t int_num;
    uint32_t error_code;

    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
    uint32_t user_esp;
    uint32_t ss;
} __attribute__((packed));

#define cli() __asm__ __volatile__("cli")
#define sti() __asm__ __volatile__("sti")

void set_irq_handler(uint8_t number, void *handler);
void init_irq();

#endif
