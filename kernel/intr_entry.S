.extern divide_error_handler

.text
.global _divide_error_entry

_divide_error_entry:
    mov %rsp, %rdi
    call divide_error_handler
    // uncomment this if Privilege Level changes
    // add $32, %rsp
    iretq
