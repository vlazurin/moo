#ifndef H_GDT
#define H_GDT

void init_gdt();
void set_kernel_stack(uint32_t stack);

#endif
