#include "ioapic.h"
#include "mem.h"
#include "printk.h"
#include "x86.h"
#include "intr.h"

static volatile u32	*ioregsel = (u32 *)0xFFFFFFFFFEC00000,
			*iowin    = (u32 *)0xFFFFFFFFFEC00010;

u32
rioreg(u32 index)
{
	*ioregsel = index;
	return *iowin;
}

void
wioreg(u32 index, u32 value)
{
	*ioregsel = index;
	*iowin = value;
}

void
wioregl(u32 index, u64 value)
{
	u32 hi = value >> 32, lo = value;
	*ioregsel = index;
	*iowin = lo;
	*ioregsel = index + 1;
	*iowin = hi;
}

void
init_ioapic()
{
	// map ioapic to kernel address
	pte_t *pte;
	walk_pgtbl(k_pgtbl, PAGEADDR((u64)ioregsel), &pte, 1);
	*pte = 0xFEC00000 | PTE_P | PTE_W | PTE_PWT | PTE_PCD;
	lcr3(rcr3());
	// log some info
	// printk("IOAPICID: %x\n", rioreg(0x0) >> 24);
	// printk("IOAPICVER: %x\n", rioreg(0x1));
	// printk("IOAPICARB: %x\n", rioreg(0x2));
	// mask all interrupts, except for keyboard
	for (int index = 0x10; index < 0x40; index += 2) {
		wioregl(index, IOAPIC_MASKED);
		// wioreg(index, 48 + (index - 0x10) / 2);
	}
	wioregl(0x12, 36);

	u64 apic_base_msr = rdmsr(0x1B);
	printk("apic_base_msr: %lx\n", apic_base_msr);
	apic_base_msr |= (1u << 11);
	stmsr(0x1B, apic_base_msr);
}