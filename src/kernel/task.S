.global enter_userspace
enter_userspace:
    mov $0x23, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    mov 8(%esp), %eax
    mov 4(%esp), %edx
    pushl $0x23
    pushl %eax
    pushf
    popl %eax
    orl $0x200, %eax
    pushl %eax
    pushl $0x1B
    push %edx
    iret

.global return_to_userspace
return_to_userspace:
    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa
    add $8, %esp
    iret

.global perform_task_switch
perform_task_switch:
    cli
    mov 4(%esp), %ecx
    mov 8(%esp), %ebp
    mov 12(%esp), %esp
    mov $0x12345, %eax
    sti
    jmp *%ecx

.global read_eip
read_eip:
    pop %eax
    jmp *%eax
