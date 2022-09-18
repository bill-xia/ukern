/*
    This file defines functions used for memory management.
*/

#include "mem.h"
#include "types.h"
#include "ukernio.h"
#include "x86.h"

struct MemInfo *k_meminfo; // read from NVRAM at boot time
struct PageInfo *k_pageinfo;
uint64_t *k_pml4, *k_pdpt, *kpgtbl_end;

static uint64_t n_pages, n_pml4, n_pdpt;
static uint64_t max_addr;

uint64_t max(uint64_t a, uint64_t b) {
    return a > b ? a : b;
}

// initiating k_pageinfo array.
void
init_kpageinfo(void)
{
    k_meminfo = (struct MemInfo *)0x9000;
    k_pageinfo = end_kmem;
	int ind = 0;
    max_addr = 0;
    while (k_meminfo[ind].type != 0) {
        if (k_meminfo[ind].type != 1) {
            ++ind;
            continue;
        }
        uint64_t beg_addr = ROUNDUP(k_meminfo[ind].paddr, PGSIZE),
                 end_addr = ROUNDDOWN(k_meminfo[ind].paddr + k_meminfo[ind].length, PGSIZE);
        printk("beg_addr: %p, end_addr: %p\n", beg_addr, end_addr);
        max_addr = max(max_addr, end_addr);
        while (beg_addr < end_addr) {
            k_pageinfo[n_pages++].paddr = beg_addr;
            beg_addr += PGSIZE;
        }
        ++ind;
    }
    end_kmem = k_pageinfo + n_pages;
}

uint64_t div_roundup(uint64_t a, uint64_t b) {
    return (a - 1) / b + 1;
}

#define NENTRY(addr, offset) div_roundup(addr, 1ull << offset)

// register an 1GB entry mapped to addr in kpgtbl
void
reg_kpgtbl_1Gpage(uint64_t addr)
{
    uint64_t pml4i = addr >> PML4_OFFSET;
    uint64_t pdpti = addr >> PDPT_OFFSET;
    k_pml4[pml4i] = k2p(&k_pdpt[pdpti & (~0x1FF)]) | PML4E_P | PML4E_W;
    k_pdpt[pdpti] = addr | PDPTE_P | PDPTE_W | PDPTE_PS;
}

// initiating kernel page table.
void
init_kpgtbl(void)
{
    k_pml4 = (uint64_t *)ROUNDUP((uint64_t)(end_kmem), PGSIZE);
    n_pml4 = NENTRY(max_addr, PML4_OFFSET);
    k_pdpt = (uint64_t *)ROUNDUP((uint64_t)(k_pml4 + n_pml4), PGSIZE);
    n_pdpt = NENTRY(max_addr, PDPT_OFFSET);
    kpgtbl_end = (uint64_t *)ROUNDUP((uint64_t)(k_pdpt + n_pdpt), PGSIZE);
    for (uint64_t i = 0; i < max_addr; i += 0x40000000) { // 1G pages
        reg_kpgtbl_1Gpage(i);
    }
    for (int i = 0; i < 256; ++i) {
        k_pml4[i + 256] = k_pml4[i];
    }
    lcr3(k2p(k_pml4));
    end_kmem = k_pdpt + n_pdpt;
}
