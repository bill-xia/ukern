#include "mp.h"
#include "x86.h"
#include "printk.h"
#include "mem.h"

// Local APIC registers, divided by 4 for use as uint32_t[] indices.
#define ID      (0x0020/4)   // ID
#define VER     (0x0030/4)   // Version
#define TPR     (0x0080/4)   // Task Priority
#define EOI     (0x00B0/4)   // EOI
#define SVR     (0x00F0/4)   // Spurious Interrupt Vector
	#define ENABLE     0x00000100   // Unit Enable
#define ESR     (0x0280/4)   // Error Status
#define ICRLO   (0x0300/4)   // Interrupt Command
	#define INIT       0x00000500   // INIT/RESET
	#define STARTUP    0x00000600   // Startup IPI
	#define DELIVS     0x00001000   // Delivery status
	#define ASSERT     0x00004000   // Assert interrupt (vs deassert)
	#define DEASSERT   0x00000000
	#define LEVEL      0x00008000   // Level triggered
	#define BCAST      0x00080000   // Send to all APICs, including self.
	#define OTHERS     0x000C0000   // Send to all APICs, excluding self.
	#define BUSY       0x00001000
	#define FIXED      0x00000000
#define ICRHI   (0x0310/4)   // Interrupt Command [63:32]
#define TIMER   (0x0320/4)   // Local Vector Table 0 (TIMER)
	#define X1         0x0000000B   // divide counts by 1
	#define PERIODIC   0x00020000   // Periodic
#define PCINT   (0x0340/4)   // Performance Counter LVT
#define LINT0   (0x0350/4)   // Local Vector Table 1 (LINT0)
#define LINT1   (0x0360/4)   // Local Vector Table 2 (LINT1)
#define ERROR   (0x0370/4)   // Local Vector Table 3 (ERROR)
	#define MASKED     0x00010000   // Interrupt masked
#define TICR    (0x0380/4)   // Timer Initial Count
#define TCCR    (0x0390/4)   // Timer Current Count
#define TDCR    (0x03E0/4)   // Timer Divide Configuration

void init_timer(void);
uint32_t *lapic;

static void
lapicw(int index, int value)
{
	lapic[index] = value;
	lapic[ID];  // wait for write to finish, by reading
}

void
init_mp(void)
{
	// outb(0x22, 0x70);
	// outb(0x23, 0x01);
    lapic = (uint32_t *)APIC_BASE;
    uint64_t rax, rbx, rcx, rdx;
    cpuid(1, &rax, &rbx, &rcx, &rdx); // read APIC info
    printk("detect APIC: %lx\n", (rdx >> 9) & 1);
	// read MSR
	uint64_t apic_base_msr = rdmsr(0x1B);
	printk("apic_base_msr: %lx\n", apic_base_msr);
    // set up APIC register mapping
    pte_t *pte;
    walk_pgtbl(k_pml4, APIC_BASE, &pte, 1);
    *pte = 0xFEE00000 | PTE_P | PTE_W | PTE_PWT | PTE_PCD;
	lcr3(rcr3());
	// info
	uint32_t ver = lapic[VER];
	printk("ver: %x\n", ver);
	uint32_t svr = lapic[SVR];
	printk("svr: %x\n", svr);
	lapicw(SVR, 0x100 | 35);
	// Init LVT
    init_timer();
    // Disable performance counter overflow interrupts
	// on machines that provide that interrupt entry.
	if (((lapic[VER]>>16) & 0xFF) >= 4)
		lapicw(PCINT, MASKED);
	// TODO: LINT0 should be used to accept hardware interrupts, rather than masked.
	lapicw(LINT0, MASKED);
    lapicw(LINT1, MASKED);
	// Map error interrupt to IRQ_ERROR.
	lapicw(ERROR, 34);

	// Clear error status register (requires back-to-back writes).
	lapicw(ESR, 0);
	lapicw(ESR, 0);

	// Ack any outstanding interrupts.
	lapicw(EOI, 0);

	// Send an Init Level De-Assert to synchronize arbitration ID's.
	// lapicw(ICRHI, 0);
	// lapicw(ICRLO, BCAST | INIT | LEVEL);
	// while(lapic[ICRLO] & DELIVS)
	// 	;

	// Enable interrupts on the APIC (but not on the processor).
	lapicw(TPR, 0);
    // sti();
}

void
init_timer(void)
{
    lapicw(TIMER, 33 | PERIODIC);
    lapicw(TDCR, X1);
    lapicw(TICR, 10000000);
}

void
lapic_eoi(void)
{
	lapicw(EOI, 0);
}