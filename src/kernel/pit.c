#include "pit.h"
#include "debug.h"
#include "system.h"
#include "port.h"
#include "interrupts.h"
#include "tasking.h"

volatile uint32_t pit_ticks;
extern volatile uint8_t task_switch_required;

IRQ_HANDLER(pit_tick_handler, 0)
{
    __sync_add_and_fetch(&pit_ticks, 1);
    task_switch_required = 1;
}

uint32_t get_pit_ticks()
{
    return pit_ticks;
}

void init_pit()
{
    uint16_t divisor = (uint16_t)(PIT_FREQUENCY / TICK_FREQUENCY);

    cli();
    set_interrupt_gate(0x20, pit_tick_handler, 0x08, 0x8E);
    outb(PIT_CMD, CMD_BINARY | CMD_MODE_2 | CMD_RW_BOTH | CMD_COUNTER_0);
    outb(PIT_COUNTER_0, divisor);
    outb(PIT_COUNTER_0, divisor >> 8);
    sti();
}
