.global read_eip

read_eip:
    pop %eax
    jmp *%eax
