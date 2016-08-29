#include "interrupts.h"
#include "debug.h"
#include "port.h"
#include "string.h"
#include "system.h"

idt_entry_t idt[256];
idt_t idt_descriptor;

IRQ_HANDLER(default_irq_handler, 0)
{
}

IRQ_HANDLER(divide_by_zero_exception, 0)
{
    debug("Divide by zero exception\n");
    hlt();
}

IRQ_HANDLER(bound_range_exception, 0)
{
    debug("Bound range exceeded exception\n");
    hlt();
}

IRQ_HANDLER(invalide_opcode_exception, 0)
{
    debug("Invalid opcode exception\n");
    hlt();
}

IRQ_HANDLER(device_not_available_exception, 0)
{
    debug("Device not available exception\n");
    hlt();
}

IRQ_HANDLER(invalid_tss_exception, 0)
{
    debug("Invalid TSS exception\n");
    hlt();
}

IRQ_HANDLER(segment_not_present_exception, 0)
{
    debug("Segment not present exception\n");
    hlt();
}

IRQ_HANDLER(stack_segment_fault, 0)
{
    debug("Stack segment fault\n");
    hlt();
}

IRQ_HANDLER(general_protection_fault, 0)
{
    debug("General protection fault\n");
    hlt();
}

IRQ_HANDLER(page_fault, 0)
{
    debug("Page fault\n");
    hlt();
}

IRQ_HANDLER(floating_point_exception, 0)
{
    debug("Floating point exception\n");
    hlt();
}

IRQ_HANDLER(aligment_check_exception, 0)
{
    debug("Aligment check exception\n");
    hlt();
}

IRQ_HANDLER(simb_floating_point_exception, 0)
{
    debug("SIMB floating point exception\n");
    hlt();
}

IRQ_HANDLER(virtualization_exception, 0)
{
    debug("Virtualization exception\n");
    hlt();
}

void send_eoi()
{
    // EOI must be sent only to right controller
    //if (current_irq == 0xFF)
    //	printf("Invalid IRQ context!\n");
    //if (current_irq > 8)
    outb(0x20, 0x20);
    outb(0xA0, 0x20);
}

void set_interrupt_gate(uint8_t number, void *handler, uint16_t selector, uint8_t flags)
{
    idt[number].base_low = (uint32_t)handler & 0xFFFF;
    idt[number].base_high = (uint32_t)handler >> 16;
    idt[number].selector = selector;
    idt[number].reserved = 0;
    idt[number].flags = flags;
}

void irq_remap()
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

void init_interrupts()
{
    irq_remap();

    memset(&idt, 0, sizeof(idt_t));
    // we need uint16_t, because uint8_t max value is 255... condition i <= 255 will give us inifinity loop (i always <= 256)
    for (uint16_t i = 0; i < 256; i++)
    {
        set_interrupt_gate(i, default_irq_handler, 0x08, 0x8E);
    }

    idt_descriptor.limit = sizeof(idt_entry_t) * 256 - 1;
    idt_descriptor.base = (uint32_t)&idt;

    set_interrupt_gate(0, divide_by_zero_exception, 0x08, 0x8E);
    set_interrupt_gate(5, bound_range_exception, 0x08, 0x8E);
    set_interrupt_gate(6, invalide_opcode_exception, 0x08, 0x8E);
    set_interrupt_gate(7, device_not_available_exception, 0x08, 0x8E);
    set_interrupt_gate(10, invalid_tss_exception, 0x08, 0x8E);
    set_interrupt_gate(11, segment_not_present_exception, 0x08, 0x8E);
    set_interrupt_gate(12, stack_segment_fault, 0x08, 0x8E);
    set_interrupt_gate(13, general_protection_fault, 0x08, 0x8E);
    set_interrupt_gate(14, page_fault, 0x08, 0x8E);
    set_interrupt_gate(16, floating_point_exception, 0x08, 0x8E);
    set_interrupt_gate(17, aligment_check_exception, 0x08, 0x8E);
    set_interrupt_gate(19, simb_floating_point_exception, 0x08, 0x8E);
    set_interrupt_gate(20, virtualization_exception, 0x08, 0x8E);
    set_interrupt_gate(80, default_irq_handler, 0x08, 0x8E);

    asm("lidt idt_descriptor");
    sti();
}
