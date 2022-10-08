//
//This file defines functions used for memory management.
//

#include "mem.h"
#include "types.h"
#include "printk.h"
#include "x86.h"
#include "errno.h"

struct MemInfo *k_meminfo; // read from NVRAM at boot time
struct PageInfo *k_pageinfo; // info for every available 4K-page
struct PageInfo *freepages = NULL;
pgtbl_t k_pml4;
pdpt_t k_pdpt;
char *end_kpgtbl;
char *end_kmem; // end of used kernel memory, in absolute address
uint64_t nfreepages;

static uint64_t n_pml4, n_pdpt;
static uint64_t max_addr;

uint64_t max(uint64_t a, uint64_t b) {
    return a > b ? a : b;
}

void
init_gdt(void)
{
    {struct SegDesc tmp = { // KERNCODE
        .type = 0x08,
        .l = 1,
        .dpl = 0,
        .s = 1,
        .p = 1
    };
    gdt[1] = tmp;}
    {struct SegDesc tmp = { // KERNDATA
        .type = 0x02,
        .l = 0,
        .dpl = 0,
        .s = 1,
        .p = 1
    };
    gdt[2] = tmp;}
    {struct SegDesc tmp = { // USERCODE
        .type = 0x08,
        .l = 1,
        .dpl = 3,
        .s = 1,
        .p = 1
    };
    gdt[3] = tmp;}
    {struct SegDesc tmp = { // USERDATA
        .type = 0x02,
        .l = 0,
        .dpl = 3,
        .s = 1,
        .p = 1
    };
    gdt[4] = tmp;}
    {struct TSS tmp = {
        .rsp0 = KSTACK,
        .rsp1 = KSTACK,
        .rsp2 = KSTACK,
    }; tss = tmp;}
    {struct TSSDesc tmp = {
        .limit1 = 104,
        .base1 = (uint64_t)&tss & 0xFFFF,
        .base2 = ((uint64_t)&tss >> 16) & 0xFF,
        .type = 9,
        .s = 0,
        .dpl = 3,
        .p = 1,
        .limit2 = 0,
        .avl = 0,
        .l = 0,
        .d_b = 0,
        .g = 0,
        .base3 = ((uint64_t)&tss >> 24) & 0xFF,
        .base4 = ((uint64_t)&tss >> 32) & 0xFFFFFFFF
    }; *(struct TSSDesc *)(gdt + 6) = tmp;}
    gdt_desc.limit = 8 * 8 - 1;
    gdt_desc.base = (uint64_t)gdt;
    lgdt(&gdt_desc);
    ltr(0x33);
}

// initiating k_pageinfo array.
void
init_kpageinfo(void)
{
    k_meminfo = (struct MemInfo *)0x9000;
    k_pageinfo = (struct PageInfo *)end_kmem;
	int ind = 0;
    max_addr = 0;
    while (k_meminfo[ind].type != 0) {
        if (k_meminfo[ind].type != 1) {
            ++ind;
            continue;
        }
        uint64_t beg_addr = ROUNDUP(k_meminfo[ind].paddr, PGSIZE),
                 end_addr = ROUNDDOWN(k_meminfo[ind].paddr + k_meminfo[ind].length, PGSIZE);
        printk("beg_addr: %lx, end_addr: %lx\n", beg_addr, end_addr);
        max_addr = max(max_addr, end_addr);
        while (beg_addr < end_addr) {
            PA2PGINFO(beg_addr)->paddr = beg_addr;
            beg_addr += PGSIZE;
        }
        ++ind;
    }
    end_kmem = (char *)(PA2PGINFO(max_addr));
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
    k_pml4[pml4i] = K2P(&k_pdpt[pdpti & (~0x1FF)]) | PML4E_P;
    k_pdpt[pdpti] = addr | PDPTE_P | PDPTE_PS;
}

// initiating kernel page table.
void
init_kpgtbl(void)
{
    k_pml4 = (pgtbl_t)ROUNDUP((uint64_t)(end_kmem), PGSIZE);
    n_pml4 = NENTRY(max_addr, PML4_OFFSET);
    k_pdpt = (pdpt_t)ROUNDUP((uint64_t)(k_pml4 + n_pml4), PGSIZE);
    n_pdpt = NENTRY(max_addr, PDPT_OFFSET);
    end_kpgtbl = (char *)ROUNDUP((uint64_t)(k_pdpt + n_pdpt), PGSIZE);
    for (uint64_t i = 0; i < max_addr; i += 0x40000000) { // 1G pages
        reg_kpgtbl_1Gpage(i);
    }
    for (int i = 0; i < 256; ++i) {
        k_pml4[i + 256] = k_pml4[i];
    }
    lcr3(K2P(k_pml4));
    end_kmem = end_kpgtbl;
}

// initiating free page list:
// the good old free allocing days are gone!
void
init_freepages(void)
{
    struct PageInfo *pginfo_end = PA2PGINFO(max_addr);
    for (struct PageInfo *pginfo = k_pageinfo; pginfo < pginfo_end; ++pginfo) {
        if (pginfo->paddr >= K2P((uint64_t)end_kmem)) {
            // for unavailable page, paddr is 0x0
            pginfo->u.next = freepages;
            freepages = pginfo;
            nfreepages++;
        } else {
            pginfo->u.ref++; // used by kernel
        }
    }
}

//
// Alloc a physical page, which means: remove it from free pages
// list, and set ref to 1. It doesn't play with page table.
// A syscall should never call this function "in raw",
// but call pgtbl_walk() instead.
//
// flags: only have one flag currently, namely FLAG_ZERO,
//        means to clear the content of the page to zeroes.
//
// returns a struct PageInfo * pointing to the page alloced.
// returns NULL on fail.
//
struct PageInfo *
alloc_page(uint64_t flags)
{
    if (freepages == NULL) return NULL;
    struct PageInfo *ret = freepages;
    freepages = freepages->u.next;
    char *mem = (char *)P2K(ret->paddr);
    if (flags & FLAG_ZERO)
        for (int i = 0; i < PGSIZE; mem[i] = 0, ++i) ;
    ret->u.ref = 1;
    nfreepages--;
    return ret;
}

//
// Free a physical page, which means: decrease ref by 1,
// and add it to free pages list if ref is 0 after decreasing.
// Panics if ref is 0.
//
void
free_page(struct PageInfo *page)
{
    if (page->u.ref == 0) {
        // TODO: should be panic() here, to be implemented
        printk("free_page(): page with ref equals zero, paddr: %p\n", page->paddr);
        while (1);
    }
    if (--page->u.ref == 0) {
        page->u.next = freepages;
        freepages = page;
        nfreepages++;
    }
}

//
// Take a walk at pgtbl according to vaddr. If `create`
// is non-zero, create new page in need. No permission is
// given on the pte of the new created page. Otherwise,
// returns NULL on failure. The kernel should make sure
// that `pgtbl` should be an available physical address,
// thus it is not checked.
//
// Pass out a pointer to the page table entry at *pte.
// Returns 0 on success, or -E_NOMEM if memory not enough.
//
int
walk_pgtbl(pgtbl_t pgtbl, uint64_t vaddr, pte_t **pte, int create)
{
    pgtbl = (pgtbl_t)PAGEKADDR((uint64_t)pgtbl);
    struct PageInfo *alloced[4] = {NULL,NULL,NULL,NULL};
    vaddr = PAGEADDR(vaddr);

    int pml4i = PML4_INDEX(vaddr);
    if (!(pgtbl[pml4i] & PML4E_P)) {
        if (!create) goto no_mem;
        alloced[0] = alloc_page(FLAG_ZERO); // unused entries must be zero
        if (alloced[0] == NULL) goto no_mem;
        // give all permission here, let caller control access rights
        // by modifying pte.
        pgtbl[pml4i] = alloced[0]->paddr | PML4E_P | PML4E_W | PML4E_U;
    }

    pdpt_t pdpt = (pdpt_t)PAGEKADDR(pgtbl[pml4i]);
    int pdpti = PDPT_INDEX(vaddr);
    if (!(pdpt[pdpti] & PDPTE_P)) {
        if (!create) goto no_mem;
        alloced[1] = alloc_page(FLAG_ZERO);
        if (alloced[1] == NULL) {
            goto no_mem;
        }
        pdpt[pdpti] = alloced[1]->paddr | PDPTE_P | PDPTE_W | PDPTE_U;
    }

    pd_t pd = (pd_t)PAGEKADDR(pdpt[pdpti]);
    int pdi = PD_INDEX(vaddr);
    if (!(pd[pdi] & PDE_P)) {
        if (!create) goto no_mem;
        alloced[2] = alloc_page(FLAG_ZERO);
        if (alloced[2] == NULL) {
            goto no_mem;
        }
        pd[pdi] = alloced[2]->paddr | PDE_P | PDE_W | PDE_U;
    }

    pt_t pt = (pt_t)PAGEKADDR(pd[pdi]);
    int pti = PT_INDEX(vaddr);
    if (!(pt[pti] & PTE_P)) {
        if (!create) goto no_mem;
        alloced[3] = alloc_page(FLAG_ZERO);
        if (alloced[3] == NULL) {
            goto no_mem;
        }
        // no permission given: let the caller do if needed
        pt[pti] = alloced[3]->paddr | PTE_P;
    }
    *pte = (pte_t *)P2K(pt + pti);
    return 0;

no_mem:
    for (int i = 0; i < 4 && alloced[i] != NULL; ++i) free_page(alloced[i]);
    return -E_NOMEM;
}

void memcpy(char *dst, char *src, uint64_t n_bytes) {
    // TODO: this is currently buggy: src[0:n_bytes] and dst[0:n_bytes] may overlap!
    for (int i = 0; i < n_bytes; ++i) {
        dst[i] = src[i];
    }
}

//
// pgtbl should be above KERNEL_BASE
// Free the pagetable with pml4 table at `pgtbl`.
// Also frees the page at pgtbl.
//
void
free_pgtbl(pgtbl_t pgtbl, uint64_t flags)
{
    pgtbl = (pgtbl_t)P2K(pgtbl);
    for (int pml4i = 0; pml4i < 256; ++pml4i) { // never touches kernel space
        if (!(pgtbl[pml4i] & PML4E_P))
            continue;
        pdpt_t pdpt = (pdpt_t)PAGEKADDR(pgtbl[pml4i]);
        for (int pdpti = 0; pdpti < 512; ++pdpti) {
            if (!(pdpt[pdpti] & PDPTE_P))
                continue;
            pd_t pd = (pd_t)PAGEKADDR(pdpt[pdpti]);
            for (int pdi = 0; pdi < 512; ++pdi) {
                if (!(pd[pdi] & PDE_P))
                    continue;
                pt_t pt = (pt_t)PAGEKADDR(pd[pdi]);
                for (int pti = 0; pti < 512; ++pti) {
                    if (!(pt[pti] & PTE_P))
                        continue;
                    uint64_t paddr = PAGEKADDR(pt[pti]);
                    if (flags & FREE_PGTBL_DECREF)
                        free_page(KA2PGINFO(paddr));
                }
                free_page(KA2PGINFO(pt));
            }
            free_page(KA2PGINFO(pd));
        }
        free_page(KA2PGINFO(pdpt));
    }
    free_page(KA2PGINFO(pgtbl));
}

/************************************************
 *                 used by fork                 *
 ************************************************/

int
copy_pgtbl(pgtbl_t dst, pgtbl_t src, uint64_t flags)
{
    src = (pgtbl_t)P2K(src);
    dst = (pgtbl_t)P2K(dst);
    struct PageInfo *pg;
    // copy user space
    for (int pml4i = 0; pml4i < 256; ++pml4i) {
        if (!(src[pml4i] & PML4E_P))
            continue;
        pg = alloc_page(FLAG_ZERO);
        dst[pml4i] = pg->paddr | PML4E_P | PML4E_W | PML4E_U;
        pdpt_t pdpt_s = (pdpt_t)PAGEKADDR(src[pml4i]);
        pdpt_t pdpt_d = (pdpt_t)PAGEKADDR(dst[pml4i]);
        for (int pdpti = 0; pdpti < 512; ++pdpti) {
            if (!(pdpt_s[pdpti] & PDPTE_P))
                continue;
            pg = alloc_page(FLAG_ZERO);
            pdpt_d[pdpti] = pg->paddr | PDPTE_P | PDPTE_W | PDPTE_U;
            pd_t pd_s = (pd_t)PAGEKADDR(pdpt_s[pdpti]);
            pd_t pd_d = (pd_t)PAGEKADDR(pdpt_d[pdpti]);
            for (int pdi = 0; pdi < 512; ++pdi) {
                if (!(pd_s[pdi] & PDE_P))
                    continue;
                pg = alloc_page(FLAG_ZERO);
                pd_d[pdi] = pg->paddr | PDE_P | PDE_W | PDE_U;
                pt_t pt_s = (pt_t)PAGEKADDR(pd_s[pdi]);
                pt_t pt_d = (pt_t)PAGEKADDR(pd_d[pdi]);
                for (int pti = 0; pti < 512; ++pti) {
                    if (!(pt_s[pti] & PTE_P))
                        continue;
                    pt_d[pti] = pt_s[pti];
                    if (flags & CPY_PGTBL_CNTREF)
                        PA2PGINFO(pt_s[pti])->u.ref++;
                }
            }
        }
    }
    // copy kernel space
    if (flags & CPY_PGTBL_WITHKSPACE) {
        for (int i = 256; i < 512; ++i) {
            dst[i] = src[i];
        }
    }
    return 0;
}

void
pgtbl_clearflags(pgtbl_t pgtbl, uint64_t flags)
{
    pgtbl = (pgtbl_t)PAGEKADDR((uint64_t)pgtbl);
    flags &= PTE_FLAGS;
    uint64_t mask = ~flags;
    for (int pml4i = 0; pml4i < 256; ++pml4i) {
        // only operate in user space
        if (!(pgtbl[pml4i] & PML4E_P))
            continue;
        pdpt_t pdpt = (pdpt_t)PAGEKADDR(pgtbl[pml4i]);
        for (int pdpti = 0; pdpti < 512; ++pdpti) {
            if (!(pdpt[pdpti] & PDPTE_P))
                continue;
            pd_t pd = (pd_t)PAGEKADDR(pdpt[pdpti]);
            for (int pdi = 0; pdi < 512; ++pdi) {
                if (!(pd[pdi] & PDE_P))
                    continue;
                pt_t pt = (pt_t)PAGEKADDR(pd[pdi]);
                for (int pti = 0; pti < 512; ++pti) {
                    if (!(pt[pti] & PTE_P))
                        continue;
                    pt[pti] &= mask;
                }
            }
        }
    }
}
