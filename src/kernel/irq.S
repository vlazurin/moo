.section .text
.align 4

.macro IRQ_NOERR index
    .global _irq\index
    _irq\index:
        push $0
        push $\index
        jmp irq_common
.endm

.macro IRQ_ERR index
    .global _irq\index
    _irq\index:
        push $\index
        jmp irq_common
.endm

// CPU exceptions
IRQ_NOERR 0
IRQ_NOERR 1
IRQ_NOERR 2
IRQ_NOERR 3
IRQ_NOERR 4
IRQ_NOERR 5
IRQ_NOERR 6
IRQ_NOERR 7
IRQ_ERR   8
IRQ_NOERR 9
IRQ_ERR   10
IRQ_ERR   11
IRQ_ERR   12
IRQ_ERR   13
IRQ_ERR   14
IRQ_NOERR 15
IRQ_NOERR 16
IRQ_ERR 17
IRQ_NOERR 18
IRQ_NOERR 19
IRQ_NOERR 20
IRQ_NOERR 21
IRQ_NOERR 22
IRQ_NOERR 23
IRQ_NOERR 24
IRQ_NOERR 25
IRQ_NOERR 26
IRQ_NOERR 27
IRQ_NOERR 28
IRQ_NOERR 29
IRQ_ERR 30
IRQ_NOERR 31

// hardware interrupts
IRQ_NOERR 32
IRQ_NOERR 33
IRQ_NOERR 34
IRQ_NOERR 35
IRQ_NOERR 36
IRQ_NOERR 37
IRQ_NOERR 38
IRQ_NOERR 39
IRQ_NOERR 40
IRQ_NOERR 41
IRQ_NOERR 42
IRQ_NOERR 43
IRQ_NOERR 44
IRQ_NOERR 45
IRQ_NOERR 46
IRQ_NOERR 47

// syscalls
IRQ_NOERR 128

.extern irq_handler

irq_common:
    pusha

    push %ds
    push %es
    push %fs
    push %gs
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    push %esp
    cld
    call irq_handler
    add $4, %esp

    pop %gs
    pop %fs
    pop %es
    pop %ds

    popa
    add $8, %esp
    iret
