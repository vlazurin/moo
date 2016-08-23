.code16gcc

cli
# long mode setup
push %ds
lgdt gdtr
mov %cr0, %eax
or $1, %al
mov %eax, %cr0
mov $0x08, %bx
mov %bx, %ds
and 0xFE, %al
mov %eax, %cr0
pop %ds
sti

# enable a20 line
# without open a20 access to upper memory can randomly crash
open_a20_loop:
inb $0x64, %al
testb $0x2, %al
jnz open_a20_loop
movb $0xdf, %al
outb %al, $0x64

cld
call main

gdt:
    .quad   0x0000000000000000
    .quad   0x00cf92000000ffff
gdtr:
    .word   .-gdt-1
    .long   gdt
