#include "log.h"
#include "mm.h"
#include "string.h"
#include "tss.h"

extern void flush_tss();
extern void flush_gdt(uintptr_t gdt);

#define GDT_SIZE 6

struct gdt_entry gdt_entries[GDT_SIZE];
struct gdt_register gdt_reg;
struct tss_entry tss_entry;

void set_kernel_stack(uint32_t stack)
{
   tss_entry.esp0 = stack;
}

static void write_tss(int num, uint16_t ss0, uint32_t esp0)
{
   uint32_t base = (uint32_t)(uintptr_t)&tss_entry;
   uint32_t limit = base + sizeof(tss_entry);

   gdt_set_gate(gdt_entries, num, base, limit, 0xE9, 0x00);

   memset(&tss_entry, 0, sizeof(tss_entry));

   tss_entry.ss0  = ss0;
   tss_entry.esp0 = esp0;

   // Here we set the cs, ss, ds, es, fs and gs entries in the TSS. These specify what
   // segments should be loaded when the processor switches to kernel mode. Therefore
   // they are just our normal kernel code/data segments - 0x08 and 0x10 respectively,
   // but with the last two bits set, making 0x0b and 0x13. The setting of these bits
   // sets the RPL (requested privilege level) to 3, meaning that this TSS can be used
   // to switch to kernel mode from ring 3.
   tss_entry.cs = 0x0b;
   tss_entry.ss = 0x13;
   tss_entry.ds = 0x13;
   tss_entry.es = 0x13;
   tss_entry.fs = 0x13;
   tss_entry.gs = 0x13;
}

void init_gdt()
{
    gdt_set_gate(gdt_entries, 0, 0, 0, 0, 0);
    // code segment, 4 kb, 32 bit opcode, supervisor
    gdt_set_gate(gdt_entries, 1, 0, 0xFFFFFFFF, 0x9A, 0xCF);
    // data segment, supervisor
    gdt_set_gate(gdt_entries, 2, 0, 0xFFFFFFFF, 0x92, 0xCF);
    // code segment, usermode
    gdt_set_gate(gdt_entries, 3, 0, 0xFFFFFFFF, 0xFA, 0xCF);
    // data segment, usermode
    gdt_set_gate(gdt_entries, 4, 0, 0xFFFFFFFF, 0xF2, 0xCF);
    write_tss(5, 0x10, 0x0);
    gdt_reg.limit = (sizeof(struct gdt_entry) * GDT_SIZE) - 1;
    gdt_reg.base = (uint32_t)&gdt_entries;

    flush_gdt((uint32_t)&gdt_reg);
    flush_tss();
}
