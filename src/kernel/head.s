movl %ebx, kernel_params
call main

.global kernel_params
kernel_params:
.long 0
