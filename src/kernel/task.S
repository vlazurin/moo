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
