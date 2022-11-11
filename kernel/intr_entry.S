.extern trap_handler

.text
.global _divide_error_entry
.global _debug_exception_entry
.global _nmi_interrupt_entry
.global _breakpoint_entry
.global _overflow_entry
.global _bound_range_exceeded_entry
.global _invalid_opcode_entry
.global _no_math_coprocessor_entry
.global _double_fault_entry
.global _coprocesor_segment_overrun_entry
.global _invalid_tss_entry
.global _segment_not_present_entry
.global _stack_segment_fault_entry
.global _general_protection_entry
.global _page_fault_entry
.global _reserved_15_entry
.global _math_fault_entry
.global _alignment_check_entry
.global _machine_check_entry
.global _simd_floating_point_exception_entry
.global _virtualization_exception_entry
.global _control_protection_exception_entry
.global _reserved_22_entry
.global _reserved_23_entry
.global _reserved_24_entry
.global _reserved_25_entry
.global _reserved_26_entry
.global _reserved_27_entry
.global _reserved_28_entry
.global _reserved_29_entry
.global _reserved_30_entry
.global _reserved_31_entry
.global _syscall_entry
.global _timer_entry
.global _apic_error_entry
.global _apic_spurious_entry
.global _apic_keyboard_entry

_divide_error_entry:
    pushq $0
    pushq $0
    jmp call_trap_handler

_debug_exception_entry:
    pushq $0
    pushq $1
    jmp call_trap_handler

_nmi_interrupt_entry:
    pushq $0
    pushq $2
    jmp call_trap_handler

_breakpoint_entry:
    pushq $0
    pushq $3
    jmp call_trap_handler

_overflow_entry:
    pushq $0
    pushq $4
    jmp call_trap_handler

_bound_range_exceeded_entry:
    pushq $0
    pushq $5
    jmp call_trap_handler

_invalid_opcode_entry:
    pushq $0
    pushq $6
    jmp call_trap_handler

_no_math_coprocessor_entry:
    pushq $0
    pushq $7
    jmp call_trap_handler

_double_fault_entry: // has errno
    pushq $0
    pushq $8
    jmp call_trap_handler

_coprocesor_segment_overrun_entry:
    pushq $0
    pushq $9
    jmp call_trap_handler

_invalid_tss_entry: // has errno
    pushq $10
    jmp call_trap_handler

_segment_not_present_entry: // has errno
    pushq $11
    jmp call_trap_handler

_stack_segment_fault_entry: // has errno
    pushq $12
    jmp call_trap_handler

_general_protection_entry: // has errno
    pushq $13
    jmp call_trap_handler

_page_fault_entry: // has errno
    pushq $14
    jmp call_trap_handler

_reserved_15_entry:
    pushq $0
    pushq $15
    jmp call_trap_handler

_math_fault_entry:
    pushq $0
    pushq $16
    jmp call_trap_handler

_alignment_check_entry: // has errno
    pushq $17
    jmp call_trap_handler

_machine_check_entry:
    pushq $0
    pushq $18
    jmp call_trap_handler

_simd_floating_point_exception_entry:
    pushq $0
    pushq $19
    jmp call_trap_handler

_virtualization_exception_entry:
    pushq $0
    pushq $20
    jmp call_trap_handler

_control_protection_exception_entry: // has errno
    pushq $21
    jmp call_trap_handler

_reserved_22_entry:
    pushq $0
    pushq $22
    jmp call_trap_handler

_reserved_23_entry:
    pushq $0
    pushq $23
    jmp call_trap_handler

_reserved_24_entry:
    pushq $0
    pushq $24
    jmp call_trap_handler

_reserved_25_entry:
    pushq $0
    pushq $25
    jmp call_trap_handler

_reserved_26_entry:
    pushq $0
    pushq $26
    jmp call_trap_handler

_reserved_27_entry:
    pushq $0
    pushq $27
    jmp call_trap_handler

_reserved_28_entry:
    pushq $0
    pushq $28
    jmp call_trap_handler

_reserved_29_entry:
    pushq $0
    pushq $29
    jmp call_trap_handler

_reserved_30_entry:
    pushq $0
    pushq $30
    jmp call_trap_handler

_reserved_31_entry:
    pushq $0
    pushq $31
    jmp call_trap_handler

_syscall_entry:
    pushq $0
    pushq $32
    jmp call_trap_handler

_timer_entry:
    pushq $0
    pushq $33
    jmp call_trap_handler

_apic_error_entry:
    pushq $0
    pushq $34
    jmp call_trap_handler

_apic_spurious_entry:
    pushq $0
    pushq $35
    jmp call_trap_handler

_apic_keyboard_entry:
    pushq $0
    pushq $36
    jmp call_trap_handler

call_trap_handler:
    push %r15
    push %r14
    push %r13
    push %r12
    push %r11
    push %r10
    push %r9
    push %r8
    pushq $0
    push %rbp
    push %rsi
    push %rdi
    push %rbx
    push %rdx
    push %rcx
    push %rax

    mov %rsp, %rdi
    // vecnum
    mov 128(%rsp), %rsi
    // errno
    mov 136(%rsp), %rdx
    call trap_handler
    jmp return_from_trap

return_from_trap:
    pop %rax
    pop %rcx
    pop %rdx
    pop %rbx
    pop %rdi
    pop %rsi
    pop %rbp
    pop %r8
    pop %r8
    pop %r9
    pop %r10
    pop %r11
    pop %r12
    pop %r13
    pop %r14
    pop %r15
    // skip errno and vecnum
    add $16, %rsp
    iretq
