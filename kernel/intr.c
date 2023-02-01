#include "intr.h"
#include "types.h"
#include "mem.h"
#include "x86.h"
#include "printk.h"
#include "proc.h"
#include "syscall.h"
#include "sched.h"
#include "lapic.h"
#include "kbd.h"

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
void _apic_spurious_entry(void); // 35
void _apic_keyboard_entry(void); // 36

void _unused_intr_37_entry(void);
void _unused_intr_38_entry(void);
void _unused_intr_39_entry(void);
void _unused_intr_40_entry(void);
void _unused_intr_41_entry(void);
void _unused_intr_42_entry(void);
void _unused_intr_43_entry(void);
void _unused_intr_44_entry(void);
void _unused_intr_45_entry(void);
void _unused_intr_46_entry(void);
void _unused_intr_47_entry(void);

void _ioapic_0_entry(void);
void _ioapic_1_entry(void);
void _ioapic_2_entry(void);
void _ioapic_3_entry(void);
void _ioapic_4_entry(void);
void _ioapic_5_entry(void);
void _ioapic_6_entry(void);
void _ioapic_7_entry(void);
void _ioapic_8_entry(void);
void _ioapic_9_entry(void);
void _ioapic_10_entry(void);
void _ioapic_11_entry(void);
void _ioapic_12_entry(void);
void _ioapic_13_entry(void);
void _ioapic_14_entry(void);
void _ioapic_15_entry(void);
void _ioapic_16_entry(void);
void _ioapic_17_entry(void);
void _ioapic_18_entry(void);
void _ioapic_19_entry(void);
void _ioapic_20_entry(void);
void _ioapic_21_entry(void);
void _ioapic_22_entry(void);
void _ioapic_23_entry(void);

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
	_syscall_entry, //32
	_timer_entry,
	_apic_error_entry,
	_apic_spurious_entry,
	_apic_keyboard_entry, //36
	_unused_intr_37_entry,
	_unused_intr_38_entry,
	_unused_intr_39_entry,
	_unused_intr_40_entry,
	_unused_intr_41_entry,
	_unused_intr_42_entry,
	_unused_intr_43_entry,
	_unused_intr_44_entry,
	_unused_intr_45_entry,
	_unused_intr_46_entry,
	_unused_intr_47_entry,
	_ioapic_0_entry, //48
	_ioapic_1_entry,
	_ioapic_2_entry,
	_ioapic_3_entry,
	_ioapic_4_entry,
	_ioapic_5_entry,
	_ioapic_6_entry,
	_ioapic_7_entry,
	_ioapic_8_entry,
	_ioapic_9_entry,
	_ioapic_10_entry,
	_ioapic_11_entry,
	_ioapic_12_entry,
	_ioapic_13_entry,
	_ioapic_14_entry,
	_ioapic_15_entry,
	_ioapic_16_entry,
	_ioapic_17_entry,
	_ioapic_18_entry,
	_ioapic_19_entry,
	_ioapic_20_entry,
	_ioapic_21_entry,
	_ioapic_22_entry,
	_ioapic_23_entry, // 71
};

void
init_intr(void)
{
	idt = (struct IDTGateDesc *)
		ROUNDUP((u64)end_kmem, sizeof(struct IDTGateDesc));
	end_kmem = (char *)(idt + 256);
	for (int i = 0; i < NIDT; ++i) {
		struct IDTGateDesc tmp = {
			.offset1 = (u64)intr_entry[i] & MASK(16),
			.seg_sel = KERN_CODE_SEL,
			.ist = 0,
			.zeros = 0,
			.type = 14, // 14 - interrupt gate, 15 - trap gate
			.S = 0,
			.DPL = 3, // user
			.P = 1,
			.offset2 = ((u64)intr_entry[i] >> 16) & MASK(16),
			.offset3 = (u64)intr_entry[i] >> 32,
			.reserved = 0
		};
		idt[i] = tmp;
	}
	for (int i = NIDT; i < 256; ++i) {
		struct IDTGateDesc tmp = {
			.offset1 = (u64)(_reserved_31_entry) & MASK(16),
			.seg_sel = KERN_CODE_SEL,
			.ist = 0,
			.zeros = 0,
			.type = 14, // 14 - interrupt gate, 15 - trap gate
			.S = 0,
			.DPL = 3, // user
			.P = 1,
			.offset2 = ((u64)(_reserved_31_entry) >> 16) & MASK(16),
			.offset3 = (u64)(_reserved_31_entry) >> 32,
			.reserved = 0
		};
		idt[i] = tmp;
	}
	struct IDTDesc tmp = {
		.limit = 16 * 256 - 1,
		.addr = (u64)idt
	};
	idt_desc = tmp;
	lidt(&idt_desc);
}

void print_tf(struct proc_context *tf)
{
	printk("rax: %lx, rcx: %lx, rdx: %lx, rbx: %lx\n", tf->rax, tf->rcx, tf->rdx, tf->rbx);
	printk("rdi: %lx, rsi: %lx, rbp: %lx\n", tf->rdi, tf->rsi, tf->rbp);
	printk("r8: %lx, r9: %lx, r10: %lx, r11: %lx\n", tf->r8, tf->r9, tf->r10, tf->r11);
	printk("r12: %lx, r13: %lx, r14: %lx, r15: %lx\n", tf->r12, tf->r13, tf->r14, tf->r15);
	printk("cs: %lx, rip: %lx, ss: %lx, rsp: %lx\n", tf->cs, tf->rip, tf->ss, tf->rsp);
}

void page_fault_handler(struct proc_context *tf, u64 errno);
int keyboard_handler(void);

void trap_handler(struct proc_context *trapframe, u64 vecnum, u64 errno)
{
	switch(vecnum) {
	case 0:
		kill_proc(curproc, -0xFF);
		sched();
		break;
	case 14:
		page_fault_handler(trapframe, errno);
		// printk("page fault solved\n");
		return;
	case 32:
		syscall(trapframe);
		return;
	case 33:
		curproc->context = *trapframe;
		curproc->state = READY;
		curproc->exec_time++;
		lapic_eoi();
		sched();
		break;
	case 36: ;
		int c = kbd_getch();
		if (c > 0 && kbd_buf_siz < 4096) {
			kbd_buffer[(kbd_buf_beg + kbd_buf_siz) % 4096] = c;
			kbd_buf_siz++;
			if (kbd_proc != NULL) {
				// first switch to its pgtbl
				lcr3(K2P(kbd_proc->pgtbl));
				// copy from kbd_buffer into user space
				u8 *dst = (u8 *)kbd_proc->context.rcx;
				int r = min(kbd_buf_siz, kbd_proc->context.rbx);
				for (int i = 0; i < r; ++i) {
					dst[i] = (u8)kbd_buffer[kbd_buf_beg++];
					if (kbd_buf_beg == 4096)
						kbd_buf_beg = 0;
					kbd_buf_siz--;
				}
				kbd_proc->context.rax = r;
				kbd_proc->state = READY;
				kbd_proc = NULL;
				// sched
				curproc->context = *trapframe;
				curproc->state = READY;
				lapic_eoi();
				sched();
			}
		}
		lapic_eoi();
		return;
	default: // ignore
		printk("ignoring interrupt %d\n", vecnum);
		print_tf(trapframe);
		if (trapframe->cs == USER_CODE_SEL) {
			kill_proc(curproc, -0xFF);
		}
		lapic_eoi();
		return;
	}
}

void page_fault_handler(struct proc_context *tf, u64 errno) {
	int r;
	if (errno != 7) {
		printk("cr2: %p\n", rcr2());
		printk("errno: %lx\n", errno);
		printk("curproc: proc[%ld]\n", curproc - procs);
		print_tf(tf);
		if (tf->cs == KERN_CODE_SEL) {
			printk("panic: page fault in kernel.\n");
			while (1);
		}
		// in user process, kill it
		kill_proc(curproc, -0xFF);
		sched();
	}
	// check copy-on-write case
	u64 vaddr = rcr2();
	pte_t *pte;
	r = walk_pgtbl(curproc->p_pgtbl, vaddr, &pte, 0);
	u64 flags = PTE_P | PTE_U | PTE_W;
	if (r == 0 && (*pte & flags) == flags) {
		// copy-on-write
		walk_pgtbl(curproc->pgtbl, vaddr, &pte, 0);
		if (PA2PGINFO(*pte)->ref == 1) {
			// just add the write flag
			*pte |= PTE_W;
		} else {
			PA2PGINFO(*pte)->ref--;
			struct page_info *page = alloc_page(0);
			char	*src = (char *)PAGEKADDR(*pte),
				*dst = (char *)PAGEKADDR(page->paddr);
			for (int i = 0; i < PGSIZE; ++i)
				dst[i] = src[i];
			*pte = page->paddr | PTE_P | PTE_U | PTE_W;
			walk_pgtbl(curproc->p_pgtbl, vaddr, &pte, 0);
			*pte = page->paddr | PTE_P | PTE_U | PTE_W;
		}
		lcr3(rcr3());
		return;
	}
	// a malicious write
	print_tf(tf);
	printk("cr2: %p\n", rcr2());
	printk("errno: %lx\n", errno);
	printk("curproc: proc[%ld]\n", curproc - procs);
	kill_proc(curproc, -0xFF);
	sched();
}

