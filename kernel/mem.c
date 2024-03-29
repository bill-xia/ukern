//
//This file defines functions used for memory management.
//

#include "mem.h"
#include "types.h"
#include "printk.h"
#include "x86.h"
#include "errno.h"
#include "string.h"
#include "proc.h"

struct seg_desc	gdt[9] = {
	{}, // empty
	{ // KERNCODE
		.type = 0x08,
		.l = 1,
		.dpl = 0,
		.s = 1,
		.p = 1
	}, { // KERNDATA
		.type = 0x02,
		.l = 0,
		.dpl = 0,
		.s = 1,
		.p = 1
	}, { // USERCODE
		.type = 0x08,
		.l = 1,
		.dpl = 3,
		.s = 1,
		.p = 1
	}, { // USERDATA
		.type = 0x02,
		.l = 0,
		.dpl = 3,
		.s = 1,
		.p = 1
	}

};
struct gdt_desc	gdt_desc;
struct tss	tss = {
	.rsp0 = KSTACK,
	.rsp1 = KSTACK,
	.rsp2 = KSTACK,
};

pgtbl_t			k_pgtbl;
struct page_info	*k_pageinfo; // info for every available 4K-page
char			*end_kmem; // end of used kernel memory, in absolute address
u64			nfreepages;

static u64		max_addr;
static pdpt_t		k_pdpt;


static const int	n_pml4_perpage = PGSIZE / sizeof(pgtbl_t),
			n_pml4 = n_pml4_perpage,
			n_pdpt_perpage = PGSIZE / sizeof(pdpt_t),
			n_pdpt = n_pml4 * n_pdpt_perpage;

static struct page_info	*freepages = NULL;

static inline
void load_segs(u64 cs)
{
	asm volatile("push %0\n\t"
		"call long_ret\n\t"
		"jmp lcs_end\n\t"
		"long_ret: lretq\n\t"
		"lcs_end: mov $0x10, %0\n\t"
		"mov %0, %%ds\n\t"
		"mov %0, %%es\n\t"
		"mov %0, %%ss\n\t"
		"mov %0, %%fs\n\t"
		"mov %0, %%gs\n\t"
		: : "r" (cs));
}

void
init_gdt(void)
{
	struct tss_desc *tss_desc = (struct tss_desc *)(gdt + 6);
	tss_desc->limit1 = 104;
	tss_desc->base1 = (u64)&tss & 0xFFFF;
	tss_desc->base2 = ((u64)&tss >> 16) & 0xFF;
	tss_desc->type = 9;
	tss_desc->s = 0;
	tss_desc->dpl = 3;
	tss_desc->p = 1;
	tss_desc->limit2 = 0;
	tss_desc->avl = 0;
	tss_desc->l = 0;
	tss_desc->d_b = 0;
	tss_desc->g = 0;
	tss_desc->base3 = ((u64)&tss >> 24) & 0xFF;
	tss_desc->base4 = ((u64)&tss >> 32) & 0xFFFFFFFF;

	gdt_desc.limit = 8 * 8 - 1;
	gdt_desc.base = (u64)gdt;
	lgdt((void *)K2P(&gdt_desc));
	load_segs(KERN_CODE_SEL);
	ltr(0x33);
}

// initiating k_pageinfo array.
void
init_kpageinfo(struct mem_map *mem_map)
{
	k_pageinfo = (struct page_info *)end_kmem;
	int	desc_size = mem_map->desc_size,
		n_desc = mem_map->map_size / desc_size;
	max_addr = 0;

	struct mem_map_desc *desc = (struct mem_map_desc *)mem_map->list;
	// printk("ndesc: %d\n", n_desc);
	for (int i = 0; i < n_desc; ++i) {
		u64 end_addr = desc->phys_start + desc->num_of_pages*PGSIZE;
		// ignore UEFI memory type, just find the max addr
		max_addr = max(max_addr, end_addr);
		desc = (struct mem_map_desc *)((u64)desc + desc_size);
	}
	for (u64 addr = 0; addr < max_addr; addr += PGSIZE) {
		PA2PGINFO(addr)->paddr = 0;
		PA2PGINFO(addr)->ref = 0;
		PA2PGINFO(addr)->next = NULL;
	}

	desc = (struct mem_map_desc *)mem_map->list;
	for (int i = 0; i < n_desc; ++i) {
		// If we don't set paddr, this page won't be inside free list。
		// See init_freepages().
		if (desc->type != EFI_CONVENTIONAL_MEMORY) {
			desc = (struct mem_map_desc *)((u64)desc + desc_size);
			continue;
		}
		// According to UEFI Spec 7.2.3 Release 2.10, phys_start must be
		// aligned on a 4KiB boundary, and num_of_pages is number of
		// *4KiB* pages.
		u64	beg_addr = desc->phys_start,
			end_addr = desc->phys_start + desc->num_of_pages*PGSIZE;
		// printk("[%lx, %lx]  ", beg_addr, end_addr);
		while (beg_addr < end_addr) {
			PA2PGINFO(beg_addr)->paddr = beg_addr;
			beg_addr += PGSIZE;
		}
		desc = (struct mem_map_desc *)((u64)desc + desc_size);
	}
	end_kmem = (char *)(PA2PGINFO(max_addr));
}

// register an 1GB entry mapped to addr in k_pgtbl
void
reg_kpgtbl_1Gpage(u64 vaddr, u64 paddr)
{
	u64 pml4i = PML4_INDEX(vaddr);
	u64 pdpti = pml4i * n_pdpt_perpage + PDPT_INDEX(vaddr);
	k_pdpt[pdpti] = paddr | PDPTE_P | PDPTE_PS | PDPTE_W;
}

u8 kstack[4096] __attribute__ (( aligned(4096)));

// initiating kernel page table.
void
init_kpgtbl(void)
{
	// Clear any write protection,
	// kernel need write access to the whole space.
	u64 cr0 = rcr0(), cr4 = rcr4();
	if (cr0 & CR0_WP) {
		// printk("CR0_WP is set, clearing.\n");
		cr0 &= ~CR0_WP;
		lcr0(cr0);
	}
	if (cr4 & CR4_SMAP) {
		// printk("CR4_SMAP is set, clearing.\n");
		cr4 &= ~CR4_SMAP;
		lcr4(cr4);
	}
	if (cr4 & (CR4_PKE | CR4_PKS)) {
		cr4 &= ~(CR4_PKE | CR4_PKS);
		lcr4(cr4);
	}

	// Allocate PDPTs for whole address space
	// Thus whole kernel space may be copied
	// by just copying k_pgtbl[256:512]

	k_pgtbl = (pgtbl_t)ROUNDUP((u64)end_kmem, PGSIZE);
	k_pdpt = (pdpt_t)ROUNDUP((u64)(k_pgtbl + n_pml4), PGSIZE);
	pd_t kstack_pd = (pd_t)ROUNDUP((u64)(k_pdpt + n_pdpt), PGSIZE);
	pt_t kstack_pt = (pt_t)(kstack_pd + PGSIZE / sizeof(u64*));
	
	char *end_kpgtbl = (char *)(kstack_pt + PGSIZE / sizeof(u64*));

	memset(k_pgtbl, 0, (u64)end_kpgtbl - (u64)k_pgtbl);
	for (int i = 0; i < n_pml4; ++i) {
		k_pgtbl[i] = K2P(k_pdpt + n_pdpt_perpage*i) | PML4E_P | PML4E_W;
	}

	for (u64 addr = 0; addr < max_addr; addr += 0x40000000ul) { // 1G pages
		reg_kpgtbl_1Gpage(addr, addr);
		reg_kpgtbl_1Gpage(KERNBASE | addr, addr);
	}

	// kernel stack
	k_pdpt[n_pdpt - 1] = (u64)K2P(kstack_pd) | PDPTE_P | PDPTE_W;
	kstack_pd[511] = (u64)K2P(kstack_pt) | PDE_P | PDE_W;
	kstack_pt[510] = (u64)K2P(kstack) | PTE_P | PTE_W;
	lcr3(K2P(k_pgtbl));

	end_kmem = end_kpgtbl;
}

// initiating free page list:
// the good old free allocing days are gone!
void
init_freepages(void)
{
	struct page_info	*last_page = NULL,
				*pginfo,
				*pginfo_end = PA2PGINFO(max_addr);
	for (pginfo = k_pageinfo; pginfo < pginfo_end; ++pginfo) {
		if (pginfo->paddr < K2P((u64)end_kmem)) {
			// used by kernel, or unavailable
			// for unavailable page, paddr is 0x0
			pginfo->ref = 1;
			pginfo->next = NULL;
			continue;
		}
		if (last_page) {
			last_page->next = pginfo;
		} else {
			freepages = pginfo;
		}
		last_page = pginfo;
		nfreepages++;
	}
	last_page->next = NULL;
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
// returns a struct page_info * pointing to the page alloced.
// returns NULL on fail.
//
struct page_info *
alloc_page(u64 flags)
{
	if (freepages == NULL) return NULL;
	struct page_info *ret = freepages;
	freepages = freepages->next;
	if (freepages->paddr > max_addr) {
		panic("alloc_page(): paddr %lx greater than max_addr %lx",
			freepages->paddr,
			max_addr);
	}
	if (flags & FLAG_ZERO) {
		memset((void *)P2K(ret->paddr), 0, PGSIZE);
	}
	ret->ref = 1;
	nfreepages--;
	return ret;
}

//
// Free a physical page, which means: decrease ref by 1,
// and add it to free pages list if ref is 0 after decreasing.
// Panics if ref is 0.
//
void
free_page(struct page_info *page)
{
	if (page->paddr < K2P((u64)end_kmem)) {
		panic("free_page(): try to free page inside kernel： %lx\n",
			page->paddr);
	}
	if (page->paddr >= K2P(max_addr)) {
		panic("free_page(): addr %lx greater than max_addr %lx.\n",
			page->paddr,
			max_addr);
	}
	if (page->ref <= 0) {
		panic("free_page(): page %p with ref(%d) <= 0\n",
			page->paddr,
			page->ref);
	}
	if (--page->ref == 0) {
		page->next = freepages;
		freepages = page;
		nfreepages++;
	}
}

// Take a walk at pgtbl according to vaddr. If `create` is non-zero, create new
// page in need, but never create final physical page. This is because we
// may want pte to map to other type of memory, e.g. MMIO. Call alloc_page() and
// assign pginfo->addr to pte if this is what you need. Higher-level page tables
// are allocated with all permission (P|U|W), thus the final permission is up to
// the caller to fill in.
//
// `pgtbl` will be converted to kernel page-aligned address.
//
// Pass out a pointer to the page table entry at *pte, pass
// NULL if we cannot reach final level of page table and create is 0.
//
// Returns:
// 	0 on success
// 	-EFAULT if vaddr is not mapped in higher level pgtbl and create is 0
// 	-ENOMEM if memory not enough
int
walk_pgtbl(pgtbl_t pgtbl, u64 vaddr, pte_t **pte, int create)
{
	pgtbl = (pgtbl_t)PAGEKADDR((u64)pgtbl);
	struct page_info *alloced[3] = {NULL,NULL,NULL};
	vaddr = PAGEADDR(vaddr);

	int pml4i = PML4_INDEX(vaddr);
	if (!(pgtbl[pml4i] & PML4E_P)) {
		if (!create) goto unmapped;
		alloced[0] = alloc_page(FLAG_ZERO); // unused entries must be 0
		if (alloced[0] == NULL) {
			goto no_mem;
		}
		// give all permission here, let caller control access rights
		// by modifying pte.
		pgtbl[pml4i] = alloced[0]->paddr | PML4E_P | PML4E_W | PML4E_U;
	}

	pdpt_t pdpt = (pdpt_t)PAGEKADDR(pgtbl[pml4i]);
	int pdpti = PDPT_INDEX(vaddr);
	assert(!(pdpt[pdpti] & PDPTE_PS)); // TODO: support walk in huge page
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
	assert(!(pd[pdi] & PDE_PS));
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
	if (pte != NULL)
		*pte = (pte_t *)P2K(pt + pti);
	return 0;
unmapped:
	if (pte != NULL)
		*pte = NULL;
	return -EFAULT;
no_mem:
	for (int i = 0; i < 3 && alloced[i] != NULL; ++i) {
		free_page(alloced[i]);
	}
	return -ENOMEM;
}

int
map_mmio(pgtbl_t pgtbl, u64 vaddr, u64 mmioaddr, pte_t **_pte)
{
	int r;
	pte_t *pte;
	if ((r = walk_pgtbl(pgtbl, vaddr, &pte, 1)) < 0) {
		return r;
	}
	*pte = mmioaddr | PTE_P | PTE_W | PTE_PWT | PTE_PCD;
	if (_pte != NULL) {
		*_pte = pte;
	}
	return 0;
}

//
// Free the pagetable with pml4 table at `pgtbl`.
// Also frees the page at pgtbl.
//
void
free_pgtbl(pgtbl_t pgtbl, u64 flags)
{
	assert(!((u64)pgtbl & MASK(PGSHIFT)));
	pgtbl = (pgtbl_t)P2K(pgtbl);
	for (int pml4i = 0; pml4i < 256; ++pml4i) { // never touch kernel space
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
					if (flags & FREE_PGTBL_DECREF) {
						free_page(PA2PGINFO(pt[pti]));
					}
				}
				free_page(KA2PGINFO(pt));
			}
			free_page(KA2PGINFO(pd));
		}
		free_page(KA2PGINFO(pdpt));
	}
	free_page(KA2PGINFO(pgtbl));
}

void
print_pgtbl(pgtbl_t pgtbl)
{
	pgtbl = (pgtbl_t)P2K(pgtbl);
	printk("pgtbl: %lx\n", pgtbl);
	for (int pml4i = 0; pml4i < 256; ++pml4i) {
		if (!(pgtbl[pml4i] & PML4E_P))
			continue;
		printk("  %d %lx\n", pml4i, pgtbl[pml4i]);
		pdpt_t pdpt = (pdpt_t)PAGEKADDR(pgtbl[pml4i]);
		for (int pdpti = 0; pdpti < 512; ++pdpti) {
			if (!(pdpt[pdpti] & PDPTE_P))
				continue;
			printk("    %d %lx\n", pml4i, pdpt[pdpti]);
			pd_t pd = (pd_t)PAGEKADDR(pdpt[pdpti]);
			for (int pdi = 0; pdi < 512; ++pdi) {
				if (!(pd[pdi] & PDE_P))
					continue;
				printk("      %d %lx\n", pdi, pd[pdi]);
				pt_t pt = (pt_t)PAGEKADDR(pd[pdi]);
				for (int pti = 0; pti < 512; ++pti) {
					if (!(pt[pti] & PTE_P))
						continue;
					printk("        %d %lx\n", pti, pt[pti]);
				}
			}
		}
	}
}

/************************************************
 *                 used by fork                 *
 ************************************************/

int
copy_pgtbl(pgtbl_t dst, pgtbl_t src, u64 flags)
{
	src = (pgtbl_t)P2K(src);
	dst = (pgtbl_t)P2K(dst);
	struct page_info *pg;
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
						PA2PGINFO(pt_s[pti])->ref++;
				}
			}
		}
	}
	// copy kernel space
	if (!(flags & CPY_PGTBL_NOKSPACE)) {
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
