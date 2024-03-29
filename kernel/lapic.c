#include "lapic.h"
#include "x86.h"
#include "printk.h"
#include "mem.h"

void init_timer(void);
volatile u32 *lapic;

static void
lapicw(int index, int value)
{
	lapic[index] = value;
	lapic[LAPICR_ID];  // wait for write to finish, by reading
}

void
print_IRR_ISR()
{
	int index = 0x100 / 4, index_end = 0x180 / 4;
	printk("ISR\n");
	while (index < index_end) {
		printk("%x ", lapic[index]);
		index += 4;
	}
	printk("\n");
	printk("IRR\n");
	index = 0x200 / 4, index_end = 0x280 / 4;
	while (index < index_end) {
		printk("%x ", lapic[index]);
		index += 4;
	}
	printk("\n");
}

void
init_mp(void)
{
	lapic = (u32 *)LAPIC_BASE;
	u64 rax, rbx, rcx, rdx;
	cpuid(1, &rax, &rbx, &rcx, &rdx); // read APIC info
	// printk("detect APIC: %lx\n", (rdx >> 9) & 1);
	// read MSR
	// u64 apic_base_msr = rdmsr(0x1B);
	// printk("apic_base_msr: %lx\n", apic_base_msr);
	// set up APIC register mapping
	pte_t *pte;
	walk_pgtbl(k_pgtbl, LAPIC_BASE, &pte, 1);
	*pte = 0xFEE00000 | PTE_P | PTE_W | PTE_PWT | PTE_PCD;
	lcr3(rcr3());
	// info
	// u32 lapicid = lapic[LAPICR_ID];
	// printk("id: %x\n", lapicid);
	// u32 ver = lapic[LAPICR_VER];
	// printk("ver: %x\n", ver);
	// u32 svr = lapic[LAPICR_SVR];
	// printk("svr: %x\n", svr);
	lapicw(LAPICR_SVR, 0x100 | 35); // spurious vector 35, APIC software enable
	// Init LVT
	init_timer();
	// Disable performance counter overflow interrupts
	// on machines that provide that interrupt entry.
	if (((lapic[LAPICR_VER]>>16) & 0xFF) >= 4)
		lapicw(LAPICR_PCINT, LAPIC_MASKED);
	// TODO: LINT0 should be used to accept hardware interrupts, rather than masked.
	lapicw(LAPICR_LINT0, LAPIC_MASKED);
	lapicw(LAPICR_LINT1, LAPIC_MASKED);
	// Map error interrupt to IRQ_ERROR.
	lapicw(LAPICR_ERROR, 34);

	// Clear error status register (requires back-to-back writes).
	lapicw(LAPICR_ESR, 0);
	lapicw(LAPICR_ESR, 0);

	// Ack any outstanding interrupts.
	for (int i = 0; i < 256; ++i) {
		lapicw(LAPICR_EOI, 0);
	}

	// Send an Init Level De-Assert to synchronize arbitration ID's.
	// lapicw(ICRHI, 0);
	// lapicw(ICRLO, BCAST | INIT | LEVEL);
	// while(lapic[ICRLO] & DELIVS)
	// 	;

	// Enable interrupts on the APIC (but not on the processor).
	lapicw(LAPICR_TPR, 0);
	// print_IRR_ISR();
	// sti();
}

void
init_timer(void)
{
	lapicw(LAPICR_TIMER, 33 | LAPIC_PERIODIC);
	lapicw(LAPICR_TDCR, LAPIC_X1);
	lapicw(LAPICR_TICR, 10000000);
}

void
lapic_eoi(void)
{
	lapicw(LAPICR_EOI, 0);
}