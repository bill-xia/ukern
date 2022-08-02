/*
    This file defines functions used for memory management.
*/

#include "mem.h"
#include "types.h"
#include "ukernio.h"
#include "x86.h"

struct MemInfo *g_mem_info;
struct PageInfo *g_page_info = 0x160000;

// --------------------------------------------------------------
// Detect machine's physical memory setup.
// --------------------------------------------------------------

void
i386_detect_memory(void)
{
    g_mem_info = 0x9000;
	int ind = 0;
    while (g_mem_info[ind].type != 0) {
        if (g_mem_info[ind].type != 1) {
            ++ind;
            continue;
        }
        print("begin at ");
        printint(g_mem_info[ind].paddr);
        print(", length ");
        printint(g_mem_info[ind].length);
        newline();
        uint64_t beg_addr = CEIL(g_mem_info[ind].paddr, PGSIZE), end_addr = FLOOR(g_mem_info[ind].paddr + g_mem_info[ind].length, PGSIZE);
        while (beg_addr < end_addr) {
            g_page_info[beg_addr / PGSIZE].paddr = beg_addr;
            g_page_info[beg_addr / PGSIZE].ref = 0;
            beg_addr += PGSIZE;
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

void init_mem()
{
    i386_detect_memory();
}