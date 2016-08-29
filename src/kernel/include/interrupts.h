#ifndef H_INTERRUPTS
#define H_INTERRUPTS

#include <stdint.h>
#include "tasking.h"

#define INTERRUPT_HANDLE_ERROR_CODE 1
#define INTERRUPT_IGNORE_ERROR_CODE 0

typedef struct idt_entry
{
    uint16_t base_low;
    uint16_t selector;
    uint8_t reserved;
    uint8_t flags;
    uint16_t base_high;
} __attribute__((packed)) idt_entry_t;

typedef struct idt
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_t;

void set_interrupt_gate(uint8_t number, void *handler, uint16_t selector, uint8_t flags);
void init_interrupts();

#define cli() __asm__ __volatile__("cli")
#define sti() __asm__ __volatile__("sti")

/*
 * cld - C code following the sysV ABI requires DF to be clear on function entry
 * dont remove .text directive... if handler will be after global pointer variable it will be placed in .bss, we need to place it
 * in right section.
 *
 * Important: EOI must be sent before "sti" command, or system can become unstable
 */
#define IRQ_HANDLER(name, handle_error_code) asm (         \
        ".data\n"						\
        #name "_error_code:\n"          \
        ".long 0\n"                     \
        #name "_handle_error_code:\n"   \
        ".byte 1\n"                     \
		".text\n"						\
		#name ":\n"              	  	\
        "cmpb $" #handle_error_code "," #name "_handle_error_code\n" \
        "jne " #name "_common\n"         \
        "xchg %eax, (%esp) \n" 					\
        "mov %eax," #name "_error_code\n" 				\
        "pop %eax \n"             \
        #name "_common:\n"              \
		"pushal \n" 					\
        "pushf \n" 					\
		"cld \n"						\
		"call __irq_" #name "\n"		\
        "cld \n"						\
        "call send_eoi \n"				\
        "sti \n"						\
        "cld \n"						\
        "call force_task_switch \n"				\
        "popf \n"						\
        "popal \n"						\
        "iretl");            			\
		extern void name();        		\
		void __irq_##name()
#endif
