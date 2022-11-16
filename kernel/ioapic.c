#include "ioapic.h"
#include "mem.h"
#include "printk.h"
#include "x86.h"

static uint32_t *ioregsel = (uint32_t *)0xFFFFFFFFFEC00000,
                *iowin = (uint32_t *)0xFFFFFFFFFEC00010;

uint32_t
rioreg(uint32_t index)
{
    *ioregsel = index;
    return *iowin;
}

void
wioreg(uint32_t index, uint32_t value)
{
    *ioregsel = index;
    *iowin = value;
}

void
wioregl(uint32_t index, uint64_t value)
{
    uint32_t hi = value >> 32, lo = value;
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
    walk_pgtbl(k_pml4, PAGEADDR((uint64_t)ioregsel), &pte, 1);
    *pte = 0xFEC00000 | PTE_P | PTE_W | PTE_PWT | PTE_PCD;
	lcr3(rcr3());
    wioreg(0x0, 0x9 << 24); // IOAPIC's ID is 9
    // log some info
    // printk("IOAPICID: %x\n", rioreg(0x0) >> 24);
    // printk("IOAPICVER: %x\n", rioreg(0x1));
    // printk("IOAPICARB: %x\n", rioreg(0x2));
    // mask all interrupts, except for keyboard
    for (int index = 0x10; index < 0x40; index += 2) {
        wioregl(index, IOAPIC_MASKED);
    }
    wioregl(0x12, 36);
}