.global flush_tss
flush_tss:
    mov $0x2B, %ax
    ltr %ax
    ret

.global flush_gdt
flush_gdt:
    mov 4(%esp), %eax
    lgdt (%eax)

    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %ss

    ljmp $0x08, $.flush
.flush:
    ret
