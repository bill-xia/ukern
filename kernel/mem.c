/*
    This file defines functions used for memory management.
*/

#include "mem.h"
#include "types.h"
#include "ukernio.h"
#include "x86.h"

struct MemInfo *k_mem_info;
struct PageInfo *k_page_info = 0x160000;
uint64_t *k_pml4, *k_pdpt, *k_pd, *k_pt;

static uint64_t n_pages;
static uint64_t max_addr;

// --------------------------------------------------------------
// Detect machine's physical memory setup.
// --------------------------------------------------------------


uint64_t max(uint64_t a, uint64_t b) {
    return a > b ? a : b;
}

void
i386_detect_memory(void)
{
    k_mem_info = 0x9000;
	int ind = 0;
    max_addr = 0;
    while (k_mem_info[ind].type != 0) {
        if (k_mem_info[ind].type != 1) {
            ++ind;
            continue;
        }
        uint64_t beg_addr = ROUNDDOWN(k_mem_info[ind].paddr, PGSIZE), end_addr = ROUNDUP(k_mem_info[ind].paddr + k_mem_info[ind].length, PGSIZE);
        max_addr = max(max_addr, end_addr);
        if (beg_addr < end_addr) {
            n_pages += (end_addr - beg_addr) / PGSIZE;
        }
        ++ind;
    }
}

// Allocate a physical memory page. Should only used in this file.
// Parameters: none
// Returns:
// - NULL: if no available page.
// - otherwise: a pointer to struct PageInfo of the allocated page.

struct PageInfo *alloc_page()
{
    return NULL;
}

void free_page(struct PageInfo *pginfo)
{
    //
}

// Have a walk in pagetable @{pgtbl}. Other function should use
// this function to allocate memory.
// This function doesn't check availability of pgtbl
// Parameters:
// - pml4e_t* pgtbl:
//     a pointer to the root page table.
// - uint64_t vaddr:
//     the address to be walked through
// - int create:
//     if 0, just take a look; if 1, create if not exists.
// Returns:
// - NULL: no entry if create == 0, or "no entry and alloc fails" if create == 1
// - otherwise: a pointer to the page table entry of vaddr

uint64_t *walk_pgtbl(pml4e_t* pgtbl, uint64_t vaddr, int create)
{
    struct PageInfo *pgtbl_page = NULL, *pdpt_page = NULL, *pd_page = NULL, *pt_page = NULL, *page = NULL;
    // create pgtbl, get pdpt
    if (pgtbl_page == NULL) {
        if (create == 0) {
            return NULL;
        }
        pgtbl_page = alloc_page();
        if (pgtbl_page == NULL) {
            goto fail;
        }
        pgtbl = (pml4e_t *)pgtbl_page->paddr;
    }
    int pml4i = PML4_INDEX(vaddr);
    pdpte_t *pdpt = pgtbl[pml4i] & PDPT_ADDR_MASK;
    // create pdpt, get pd
    if ((pgtbl[pml4i] & PML4E_P) == 0) {
        if (create == 0) {
            return NULL;
        }
        pdpt_page = alloc_page();
        if (pdpt_page == NULL) {
            goto fail;
        }
        pgtbl[pml4i] = pdpt_page->paddr | PML4E_P | PML4E_W;
        pdpt = (pdpte_t *)pdpt_page->paddr;
    }
    int pdpti = PDPT_INDEX(vaddr);
    pde_t *pd = pdpt[pdpti] & PD_ADDR_MASK;
    // create pd, get pt
    if ((pdpt[pdpti] & PDPTE_P) == 0) {
        if (create == 0) {
            return NULL;
        }
        pd_page = alloc_page();
        if (pd_page == NULL) {
            goto fail;
        }
        pdpt[pdpti] = pd_page->paddr | PDPTE_P | PDPTE_W;
        pd = (pdpte_t *)pd_page->paddr;
    }
    int pdi = PD_INDEX(vaddr);
    pte_t *pt = pd[pdi] & PT_ADDR_MASK;
    // create pt, get page
    if ((pd[pdi] & PDE_P) == 0) {
        if (create == 0) {
            return NULL;
        }
        pt_page = alloc_page();
        if (pt_page == NULL) {
            goto fail;
        }
        pd[pdi] = pt_page->paddr | PDE_P | PDE_W;
        pt = (pdpte_t *)pt_page->paddr;
    }
    int pti = PT_INDEX(vaddr);
    // create page
    if ((pt[pti] & PTE_P) == 0) {
        if (create == 0) {
            return NULL;
        }
        page = alloc_page();
        if (page == NULL) {
            goto fail;
        }
        pt[pti] = page->paddr | PTE_P | PTE_W;
    }
    pte_t *ret = pt + pti;
    return ret;
fail:
    if (pgtbl_page != NULL) free_page(pgtbl_page);
    if (pdpt_page != NULL) free_page(pdpt_page);
    if (pd_page != NULL) free_page(pd_page);
    if (pt_page != NULL) free_page(pt_page);
    if (page != NULL) free_page(page);
    return NULL;
}

// Initiating page table and PageInfo array.

uint64_t div_roundup(uint64_t a, uint64_t b) {
    return (a - 1) / b + 1;
}

#define NENTRY(addr, offset) div_roundup(addr, 1ull << offset)
#define NPAGES(addr, offset) div_roundup((div_roundup(addr, 1ull << offset)) * 8, PGSIZE)

void reg_kpgtbl(uint64_t addr)
{
    addr = ROUNDDOWN(addr, PGSIZE);
    int pml4i = PML4_INDEX(addr);
    int pdpti = PDPT_INDEX(addr);
    int pdi = PD_INDEX(addr);
    int pti = PT_INDEX(addr);
    k_pml4[pml4i] = (uint64_t)(&k_pdpt[pdpti]) | PML4E_P | PML4E_W;
    k_pdpt[pdpti] = (uint64_t)(&k_pd[pdi]) | PDPTE_P | PDPTE_W;
    k_pd[pdi] = (uint64_t)(&k_pt[pti]) | PDE_P | PDE_W;
    k_pt[pti] = addr | PTE_P | PTE_W;
}

void init_mem()
{
    i386_detect_memory();
    // now we need max_addr / 2^12 + max_addr / 2^21
    // + max_addr / 2^30 + max_addr / 2^39 entries
    // take max_addr / 2^12 as an example.
    // each page have an entry in the page table,
    // which is 8 bytes, takes max_addr / 2^21 * 8
    // bytes in total, so takes
    // max_addr / 2^21 * 8 / 4096 pages
    // in which divide means ceil.
    extern char end[];
    k_pml4 = ROUNDUP((uint64_t)end, PGSIZE);
    int n_pml4 = NENTRY(max_addr, PML4_OFFSET);
    k_pdpt = ROUNDUP((uint64_t)(k_pml4 + n_pml4), PGSIZE);
    int n_pdpt = NENTRY(max_addr, PDPT_OFFSET);
    k_pd = ROUNDUP((uint64_t)(k_pdpt + n_pdpt), PGSIZE);
    int n_pd = NENTRY(max_addr, PD_OFFSET);
    k_pt = ROUNDUP((uint64_t)(k_pd + n_pd), PGSIZE);
    int n_pt = NENTRY(max_addr, PT_OFFSET);
    k_page_info = ROUNDUP((uint64_t)(k_pt + n_pt), PGSIZE);
    int n_pages_kpgtbl = n_pml4 * 8 / PGSIZE + n_pdpt * 8 / PGSIZE + n_pd * 8 / PGSIZE + n_pt * 8 / PGSIZE;
    int n_pages_kmeminfo = div_roundup(n_pages * sizeof(struct PageInfo), PGSIZE);
    // uint64_t max_mapped_addr = 0x200000;

    for (uint64_t i = 0; i < ROUNDUP((uint64_t)(k_page_info + n_pages), PGSIZE); i += PGSIZE) {
        reg_kpgtbl(i);
    }
    // enable_paging();
    printf("k_pml4: %p, n_pml4: %d\n", k_pml4, n_pml4);
    printf("k_pdpt: %p, n_pdpt: %d\n", k_pdpt, n_pdpt);
    printf("k_pd: %p, n_pd: %d\n", k_pd, n_pd);
    printf("k_pt: %p, n_pt: %d\n", k_pt, n_pt);
}