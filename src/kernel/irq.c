#include "irq.h"
#include "log.h"
#include "task.h"
#include "port.h"
#include "string.h"
#include "system.h"

static struct idt_entry idt[256];
static struct idt idt_descriptor;

typedef void (*isr_t)(struct regs *);
isr_t irq_handlers[256];

extern void _irq0();
extern void _irq1();
extern void _irq2();
extern void _irq3();
extern void _irq4();
extern void _irq5();
extern void _irq6();
extern void _irq7();
extern void _irq8();
extern void _irq9();
extern void _irq10();
extern void _irq11();
extern void _irq12();
extern void _irq13();
extern void _irq14();
extern void _irq15();
extern void _irq16();
extern void _irq17();
extern void _irq18();
extern void _irq19();
extern void _irq20();
extern void _irq21();
extern void _irq22();
extern void _irq23();
extern void _irq24();
extern void _irq25();
extern void _irq26();
extern void _irq27();
extern void _irq28();
extern void _irq29();
extern void _irq30();
extern void _irq31();
extern void _irq32();
extern void _irq33();
extern void _irq34();
extern void _irq35();
extern void _irq36();
extern void _irq37();
extern void _irq38();
extern void _irq39();
extern void _irq40();
extern void _irq41();
extern void _irq42();
extern void _irq43();
extern void _irq44();
extern void _irq45();
extern void _irq46();
extern void _irq47();
extern void _irq128();

static const char *ex_messages[32] = {
    "Divide by zero error",
    "Debug",
    "Non maskable interrupt",
    "Breakpoint",
    "Overflow",
    "Bound range exceeded",
    "Invalid opcode",
    "Device not available",
    "Double fault"
    "Coprocessor Segment Overrun", // legacy
    "Invalid TSS",
    "Segment not present",
    "Stack segment fault",
    "General protection fault",
    "Page fault",
    "Reserved",
    "x87 floating point exception",
    "Aligment check",
    "Machine check"
    "SIMB floating point exception",
    "Virtualization exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Security Exception",
    "Reserved"
};

void irq_handler(struct regs *r)
{
    sti(); // TODO: is it safe to do sti() here? Nested IRQ support must be implemented
    if (irq_handlers[r->int_num] != NULL) {
        irq_handlers[r->int_num](r);
    } else if (r->int_num < 32) {
        log(KERN_FATAL, "unhandled CPU exception: %s, EIP: %x\n", ex_messages[r->int_num], r->eip);
        hlt();
    }

    if (r->int_num >= 32 || r->int_num <= 47) {
        if (r->int_num >= 40) {
            outb(0xA0, 0x20);
        }
        outb(0x20, 0x20);
    }
    switch_task();
}

static void set_irq_gate(uint8_t number, void *handler)
{
    idt[number].base_low = (uint32_t)handler & 0xFFFF;
    idt[number].base_high = (uint32_t)handler >> 16;
    idt[number].selector = 0x08;
    idt[number].reserved = 0;
    idt[number].flags = 0x8E | 0x60;
}

void set_irq_handler(uint8_t number, void *handler)
{
    irq_handlers[number] = handler;
}

static void irq_remap()
{
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0x0);
    outb(0xA1, 0x0);
}

void init_irq()
{
    irq_remap();

    memset(&irq_handlers, 0, sizeof(isr_t) * 256);

    memset(&idt, 0, sizeof(struct idt));
    idt_descriptor.limit = sizeof(struct idt_entry) * 256 - 1;
    idt_descriptor.base = (uint32_t)&idt;

    set_irq_gate(0, _irq0);
    set_irq_gate(1, _irq1);
    set_irq_gate(2, _irq2);
    set_irq_gate(3, _irq3);
    set_irq_gate(4, _irq4);
    set_irq_gate(5, _irq5);
    set_irq_gate(6, _irq6);
    set_irq_gate(7, _irq7);
    //set_irq_gate(8, _irq8);
    set_irq_gate(9, _irq9);
    set_irq_gate(10, _irq10);
    set_irq_gate(11, _irq11);
    set_irq_gate(12, _irq12);
    set_irq_gate(13, _irq13);
    //set_irq_gate(14, _irq14);
    set_irq_gate(15, _irq15);
    set_irq_gate(16, _irq16);
    set_irq_gate(17, _irq17);
    set_irq_gate(18, _irq18);
    set_irq_gate(19, _irq19);
    set_irq_gate(20, _irq20);
    set_irq_gate(21, _irq21);
    set_irq_gate(22, _irq22);
    set_irq_gate(23, _irq23);
    set_irq_gate(24, _irq24);
    set_irq_gate(25, _irq25);
    set_irq_gate(26, _irq26);
    set_irq_gate(27, _irq27);
    set_irq_gate(28, _irq28);
    set_irq_gate(29, _irq29);
    set_irq_gate(30, _irq30);
    set_irq_gate(31, _irq31);
    set_irq_gate(32, _irq32);
    set_irq_gate(33, _irq33);
    set_irq_gate(34, _irq34);
    set_irq_gate(35, _irq35);
    set_irq_gate(36, _irq36);
    set_irq_gate(37, _irq37);
    set_irq_gate(38, _irq38);
    set_irq_gate(39, _irq39);
    set_irq_gate(40, _irq40);
    set_irq_gate(41, _irq41);
    set_irq_gate(42, _irq42);
    set_irq_gate(43, _irq43);
    set_irq_gate(44, _irq44);
    set_irq_gate(45, _irq45);
    set_irq_gate(46, _irq46);
    set_irq_gate(47, _irq47);
    set_irq_gate(128, _irq128);

    asm("lidt idt_descriptor");
    sti();
}
