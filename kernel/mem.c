//
//This file defines functions used for memory management.
//

#include "mem.h"
#include "types.h"
#include "printk.h"
#include "x86.h"
#include "errno.h"

struct PageInfo *k_pageinfo; // info for every available 4K-page
struct PageInfo *freepages = NULL;
pgtbl_t k_pgtbl;
pdpt_t k_pdpt;
char *end_kpgtbl;
char *end_kmem; // end of used kernel memory, in absolute address
u64 nfreepages;
struct SegDesc gdt[9];
struct GdtDesc gdt_desc;
struct TSS tss;

static u64 n_pml4, n_pdpt;
static u64 max_addr;

u64 max(u64 a, u64 b) {
	return a > b ? a : b;
}

static inline
void lcs(u64 cs)
{
	asm volatile("push %0\n\tcall long_ret\n\t jmp lcs_end\n\tlong_ret: lretq\n\tlcs_end:" : : "r" (cs));
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
		.base1 = (u64)&tss & 0xFFFF,
		.base2 = ((u64)&tss >> 16) & 0xFF,
		.type = 9,
		.s = 0,
		.dpl = 3,
		.p = 1,
		.limit2 = 0,
		.avl = 0,
		.l = 0,
		.d_b = 0,
		.g = 0,
		.base3 = ((u64)&tss >> 24) & 0xFF,
		.base4 = ((u64)&tss >> 32) & 0xFFFFFFFF
	}; *(struct TSSDesc *)(gdt + 6) = tmp;}
	gdt_desc.limit = 8 * 8 - 1;
	gdt_desc.base = K2P(gdt);
	lgdt((void *)K2P(&gdt_desc));
	lcs(KERN_CODE_SEL);
	ltr(0x33);
}

// initiating k_pageinfo array.
void
init_kpageinfo(struct mem_map *mem_map)
{
	struct mem_map_desc *desc;

	k_pageinfo = (struct PageInfo *)end_kmem;
	int	ind = 0,
		n_desc = mem_map->map_size / mem_map->desc_size;
	max_addr = 0;
	// printk("ndesc: %d\n", n_desc);
	for (ind = 0; ind < n_desc; ++ind) {
		desc = (struct mem_map_desc *)(mem_map->list + ind * mem_map->desc_size);
		max_addr = max(max_addr, desc->phys_start + desc->num_of_pages * PGSIZE);
		// printk("addr: [%lx, %lx] %d ", desc->phys_start, desc->phys_start + desc->num_of_pages * PGSIZE, desc->type);
		// if (ind % 5 == 4) {
		// 	printk("\n");
		// }
	}
	for (u64 addr = 0; addr < max_addr; addr += PGSIZE) {
		PA2PGINFO(addr)->paddr = 0;
	}
	for (ind = 0; ind < n_desc; ++ind) {
		desc = (struct mem_map_desc *)(mem_map->list + ind * mem_map->desc_size);
		// According to UEFI Spec 7.2.3 Release 2.10, phys_start must be aligned
		// on a 4KiB boundary, and num_of_pages is number of *4KiB* pages.
		u64	beg_addr = desc->phys_start,
			end_addr = desc->phys_start + desc->num_of_pages * PGSIZE;
		while (beg_addr < end_addr) {
			PA2PGINFO(beg_addr)->paddr = beg_addr;
			beg_addr += PGSIZE;
		}
	}
	end_kmem = (char *)(PA2PGINFO(max_addr));
}

u64 div_roundup(u64 a, u64 b) {
	return (a - 1) / b + 1;
}

#define NENTRY(addr, offset) div_roundup(addr, 1ull << offset)

// register an 1GB entry mapped to addr in kpgtbl
void
reg_kpgtbl_1Gpage(u64 addr)
{
	u64 pml4i = addr >> PML4_OFFSET;
	u64 pdpti = addr >> PDPT_OFFSET;
	k_pgtbl[pml4i] = K2P(&k_pdpt[pdpti & (~0x1FF)]) | PML4E_P | PML4E_W;
	k_pdpt[pdpti] = addr | PDPTE_P | PDPTE_PS | PDPTE_W;
}

u8 kstack[4096] __attribute__ (( aligned(4096)));

// initiating kernel page table.
void
init_kpgtbl(void)
{
	printk("end_kmem: %lx\n", end_kmem);
	k_pgtbl = (pgtbl_t)ROUNDUP((u64)(end_kmem), PGSIZE);
	n_pml4 = NENTRY(max_addr, PML4_OFFSET);
	k_pdpt = (pdpt_t)ROUNDUP((u64)(k_pgtbl + n_pml4), PGSIZE);
	n_pdpt = NENTRY(max_addr, PDPT_OFFSET);
	pdpt_t kstack_pdpt = (pdpt_t)ROUNDUP((u64)(k_pdpt + n_pdpt), PGSIZE);
	pd_t kstack_pd = kstack_pdpt + PGSIZE / sizeof(u64*);
	pt_t kstack_pt = kstack_pd + PGSIZE / sizeof(u64*);
	end_kpgtbl = (char *)(kstack_pt + PGSIZE / sizeof(u64*));
	u64 *ptr = (u64 *)k_pgtbl;
	while (ptr < (u64 *)end_kpgtbl) {
		*ptr = 0;
		ptr++;
	}
	for (u64 i = 0; i < max_addr; i += 0x40000000ul) { // 1G pages
		reg_kpgtbl_1Gpage(i);
	}
	for (int i = 0; i < 255; ++i) {
		k_pgtbl[i + 256] = k_pgtbl[i];
	}
	k_pgtbl[511] = (u64)K2P(kstack_pdpt) | PML4E_P | PML4E_W; // kernel stack
	kstack_pdpt[511] = (u64)K2P(kstack_pd) | PDPTE_P | PDPTE_W; // kernel stack
	kstack_pd[511] = (u64)K2P(kstack_pt) | PDE_P | PDE_W; // kernel stack
	kstack_pt[510] = (u64)K2P(kstack) | PTE_P | PTE_W; // kernel stack
	lcr3(K2P(k_pgtbl));
	end_kmem = end_kpgtbl;
}

// initiating free page list:
// the good old free allocing days are gone!
void
init_freepages(void)
{
	struct PageInfo *last_page = NULL, *pginfo_end = PA2PGINFO(max_addr);
	for (struct PageInfo *pginfo = k_pageinfo; pginfo < pginfo_end; ++pginfo) {
		if (pginfo->paddr >= K2P((u64)end_kmem)) {
			// for unavailable page, paddr is 0x0
			if (last_page) {
				last_page->u.next = pginfo;
			} else {
				freepages = pginfo;
			}
			last_page = pginfo;
			nfreepages++;
		} else {
			pginfo->u.ref++; // used by kernel
		}
	}
	last_page->u.next = NULL;
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
alloc_page(u64 flags)
{
	if (freepages == NULL) return NULL;
	struct PageInfo *ret = freepages;
	freepages = freepages->u.next;
	u64 *mem = (u64 *)P2K(ret->paddr);
	if (flags & FLAG_ZERO)
		for (int i = 0; i < PGSIZE / sizeof(u64); mem[i] = 0, ++i) ;
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
		panic("free_page(): page with ref equals zero, paddr: %p\n", page->paddr);
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
// Pass out a pointer to the page table entry at *pte, pass
// NULL if vaddr is not mapped.
// Returns 0 on success, or -E_NOMEM if memory not enough.
//
int
walk_pgtbl(pgtbl_t pgtbl, u64 vaddr, pte_t **pte, int create)
{
	pgtbl = (pgtbl_t)PAGEKADDR((u64)pgtbl);
	struct PageInfo *alloced[4] = {NULL,NULL,NULL,NULL};
	vaddr = PAGEADDR(vaddr);

	int pml4i = PML4_INDEX(vaddr);
	if (!(pgtbl[pml4i] & PML4E_P)) {
		if (!create) goto unmapped;
		alloced[0] = alloc_page(FLAG_ZERO); // unused entries must be zero
		if (alloced[0] == NULL) goto no_mem;
		// give all permission here, let caller control access rights
		// by modifying pte.
		pgtbl[pml4i] = alloced[0]->paddr | PML4E_P | PML4E_W | PML4E_U;
	}

	pdpt_t pdpt = (pdpt_t)PAGEKADDR(pgtbl[pml4i]);
	int pdpti = PDPT_INDEX(vaddr);
	if (!(pdpt[pdpti] & PDPTE_P)) {
		if (!create) goto unmapped;
		alloced[1] = alloc_page(FLAG_ZERO);
		if (alloced[1] == NULL) {
			goto no_mem;
		}
		pdpt[pdpti] = alloced[1]->paddr | PDPTE_P | PDPTE_W | PDPTE_U;
	}

	pd_t pd = (pd_t)PAGEKADDR(pdpt[pdpti]);
	int pdi = PD_INDEX(vaddr);
	if (!(pd[pdi] & PDE_P)) {
		if (!create) goto unmapped;
		alloced[2] = alloc_page(FLAG_ZERO);
		if (alloced[2] == NULL) {
			goto no_mem;
		}
		pd[pdi] = alloced[2]->paddr | PDE_P | PDE_W | PDE_U;
	}

	pt_t pt = (pt_t)PAGEKADDR(pd[pdi]);
	int pti = PT_INDEX(vaddr);
	if (!(pt[pti] & PTE_P)) {
		if (!create) goto unmapped;
		alloced[3] = alloc_page(FLAG_ZERO);
		if (alloced[3] == NULL) {
			goto no_mem;
		}
		// no permission given: let the caller do if needed
		pt[pti] = alloced[3]->paddr | PTE_P;
	}
	if (pte != NULL)
		*pte = (pte_t *)P2K(pt + pti);
	return 0;
unmapped:
	if (pte != NULL)
		*pte = NULL;
no_mem:
	for (int i = 0; i < 4 && alloced[i] != NULL; ++i) free_page(alloced[i]);
	return -E_NOMEM;
}

int
map_mmio(pgtbl_t pgtbl, u64 vaddr, u64 mmioaddr, pte_t **_pte)
{
	int r;
	pte_t *pte;
	if (r = walk_pgtbl(pgtbl, vaddr, &pte, 1)) {
		return r;
	}
	free_page(PA2PGINFO(PAGEADDR(*pte)));
	*pte = mmioaddr | PTE_P | PTE_PWT | PTE_PCD;
	if (_pte != NULL) {
		*_pte = pte;
	}
	return 0;
}

//
// pgtbl should be above KERNEL_BASE
// Free the pagetable with pml4 table at `pgtbl`.
// Also frees the page at pgtbl.
//
void
free_pgtbl(pgtbl_t pgtbl, u64 flags)
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
					u64 paddr = PAGEKADDR(pt[pti]);
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
copy_pgtbl(pgtbl_t dst, pgtbl_t src, u64 flags)
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
pgtbl_clearflags(pgtbl_t pgtbl, u64 flags)
{
	pgtbl = (pgtbl_t)PAGEKADDR((u64)pgtbl);
	flags &= PTE_FLAGS;
	u64 mask = ~flags;
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
