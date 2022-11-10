#include "intr.h"
#include "types.h"
#include "mem.h"
#include "x86.h"
#include "printk.h"
#include "proc.h"
#include "syscall.h"
#include "sched.h"
#include "mp.h"

struct IDTGateDesc *idt;
struct IDTDesc idt_desc;

void _divide_error_entry(void); // 0
void _debug_exception_entry(void); // 1
void _nmi_interrupt_entry(void); // 2
void _breakpoint_entry(void); // 3
void _overflow_entry(void); // 4
void _bound_range_exceeded_entry(void); // 5
void _invalid_opcode_entry(void); // 6
void _no_math_coprocessor_entry(void); // 7
void _double_fault_entry(void); // 8
void _coprocesor_segment_overrun_entry(void); // 9
void _invalid_tss_entry(void); // 10
void _segment_not_present_entry(void); // 11
void _stack_segment_fault_entry(void); // 12
void _general_protection_entry(void); // 13
void _page_fault_entry(void); // 14
void _reserved_15_entry(void); // 15
void _math_fault_entry(void); // 16
void _alignment_check_entry(void); // 17
void _machine_check_entry(void); // 18
void _simd_floating_point_exception_entry(void); // 19
void _virtualization_exception_entry(void); // 20
void _control_protection_exception_entry(void); // 21
void _reserved_22_entry(void); // 22
void _reserved_23_entry(void); // 23
void _reserved_24_entry(void); // 24
void _reserved_25_entry(void); // 25
void _reserved_26_entry(void); // 26
void _reserved_27_entry(void); // 27
void _reserved_28_entry(void); // 28
void _reserved_29_entry(void); // 29
void _reserved_30_entry(void); // 30
void _reserved_31_entry(void); // 31
void _syscall_entry(void); // 32
void _timer_entry(void); // 33
void _apic_error_entry(void); // 34
void _apic_spurious_entry(void); // 34

void (*intr_entry[])(void) = {
    _divide_error_entry,
    _debug_exception_entry,
    _nmi_interrupt_entry,
    _breakpoint_entry,
    _overflow_entry,
    _bound_range_exceeded_entry,
    _invalid_opcode_entry,
    _no_math_coprocessor_entry,
    _double_fault_entry,
    _coprocesor_segment_overrun_entry,
    _invalid_tss_entry,
    _segment_not_present_entry,
    _stack_segment_fault_entry,
    _general_protection_entry,
    _page_fault_entry,
    _reserved_15_entry,
    _math_fault_entry,
    _alignment_check_entry,
    _machine_check_entry,
    _simd_floating_point_exception_entry,
    _virtualization_exception_entry,
    _control_protection_exception_entry,
    _reserved_22_entry,
    _reserved_23_entry,
    _reserved_24_entry,
    _reserved_25_entry,
    _reserved_26_entry,
    _reserved_27_entry,
    _reserved_28_entry,
    _reserved_29_entry,
    _reserved_30_entry,
    _reserved_31_entry,
    _syscall_entry,
    _timer_entry,
    _apic_error_entry,
    _apic_spurious_entry
};

void
init_intr(void)
{
    idt = (struct IDTGateDesc *)
        ROUNDUP((uint64_t)end_kmem, sizeof(struct IDTGateDesc));
    end_kmem = (char *)(idt + NIDT);
    for (int i = 0; i < NIDT; ++i) {
        struct IDTGateDesc tmp = {
            .offset1 = (uint64_t)intr_entry[i] & MASK(16),
            .seg_sel = KERN_CODE_SEL,
            .ist = 0,
            .zeros = 0,
            .type = 14, // 14 - interrupt gate, 15 - trap gate
            .S = 0,
            .DPL = 3, // user
            .P = 1,
            .offset2 = ((uint64_t)intr_entry[i] >> 16) & MASK(16),
            .offset3 = (uint64_t)intr_entry[i] >> 32,
            .reserved = 0
        };
        idt[i] = tmp;
    }
    for (int i = NIDT; i < 256; ++i) {
        struct IDTGateDesc tmp = {
            .offset1 = (uint64_t)(_reserved_31_entry) & MASK(16),
            .seg_sel = KERN_CODE_SEL,
            .ist = 0,
            .zeros = 0,
            .type = 14, // 14 - interrupt gate, 15 - trap gate
            .S = 0,
            .DPL = 3, // user
            .P = 1,
            .offset2 = ((uint64_t)(_reserved_31_entry) >> 16) & MASK(16),
            .offset3 = (uint64_t)(_reserved_31_entry) >> 32,
            .reserved = 0
        };
        idt[i] = tmp;
    }
    struct IDTDesc tmp = {
        .limit = 16 * 256 - 1,
        .addr = (uint64_t)idt
    };
    idt_desc = tmp;
    lidt(&idt_desc);
}

void print_tf(struct ProcContext *tf)
{
    printk("rax: %lx, rcx: %lx, rdx: %lx, rbx: %lx\n", tf->rax, tf->rcx, tf->rdx, tf->rbx);
    printk("rdi: %lx, rsi: %lx, rbp: %lx\n", tf->rdi, tf->rsi, tf->rbp);
    printk("r8: %lx, r9: %lx, r10: %lx, r11: %lx\n", tf->r8, tf->r9, tf->r10, tf->r11);
    printk("r12: %lx, r13: %lx, r14: %lx, r15: %lx\n", tf->r12, tf->r13, tf->r14, tf->r15);
    printk("cs: %lx, rip: %lx, ss: %lx, rsp: %lx\n", tf->cs, tf->rip, tf->ss, tf->rsp);
}

void page_fault_handler(struct ProcContext *tf, uint64_t errno);

void trap_handler(struct ProcContext *trapframe, uint64_t vecnum, uint64_t errno)
{
    if (vecnum == 32) {
        syscall(trapframe);
        return;
    }
    if (vecnum == 0) {
        kill_proc(curproc);
        sched();
    }
    if (vecnum == 14) {
        page_fault_handler(trapframe, errno);
        // printk("page fault solved\n");
        return;
    }
    if (vecnum == 33) {
        lapic_eoi();
        curproc->context = *trapframe;
        curproc->state = READY;
        sched();
    }
    printk("trap handler\n");
    printk("trapframe: %p, vecnum: %ld, errno: %lx\n", trapframe, vecnum, errno);
    print_tf(trapframe);
    while (1);
}

void page_fault_handler(struct ProcContext *tf, uint64_t errno) {
    if (errno != 7) {
        printk("cr2: %p\n", rcr2());
        printk("errno: %lx\n", errno);
        printk("curproc: proc[%ld]\n", curproc - procs);
        kill_proc(curproc);
        sched();
    }
    // check copy-on-write case
    uint64_t vaddr = rcr2();
    pte_t *pte;
    walk_pgtbl(curproc->p_pgtbl, vaddr, &pte, 0);
    if ((*pte & PTE_P)
    &&  (*pte & PTE_U)
    &&  (*pte & PTE_W)) {
        // copy-on-write
        walk_pgtbl(curproc->pgtbl, vaddr, &pte, 0);
        if (PA2PGINFO(*pte)->u.ref == 1) {
            // just add the write flag
            *pte |= PTE_W;
        } else {
            PA2PGINFO(*pte)->u.ref--;
            struct PageInfo *page = alloc_page(0);
            char *src = (char *)PAGEKADDR(*pte), *dst = (char *)PAGEKADDR(page->paddr);
            for (int i = 0; i < PGSIZE; ++i) dst[i] = src[i];
            *pte = page->paddr | PTE_P | PTE_U | PTE_W;
        }
        lcr3(rcr3());
        return;
    }
    printk("cr2: %p\n", rcr2());
    printk("errno: %lx\n", errno);
    printk("curproc: proc[%ld]\n", curproc - procs);
    kill_proc(curproc);
    sched();
}