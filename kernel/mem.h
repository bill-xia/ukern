/*
 This file declares macros and functions used for memory management.
*/

#ifndef MEM_H
#define MEM_H

#include "types.h"

#define PGSIZE		4096
#define PGSHIFT		12
#define PGMASK		MASK(PGSHIFT)

#define PML4E_P		0x1
#define PML4E_W		0x2
#define PML4E_U		0x4

#define PDPTE_P		0x1
#define PDPTE_W		0x2
#define PDPTE_U		0x4
#define PDPTE_PS	0x80

#define PDE_P		0x1
#define PDE_W		0x2
#define PDE_U		0x4
#define PDE_PS		0x80

#define PTE_P		0x1
#define PTE_W		0x2
#define PTE_U		0x4
#define PTE_FLAGS	0x7
#define PTE_PWT		0x8
#define PTE_PCD		0x10

#define PML4_OFFSET	39
#define PDPT_OFFSET	30
#define PD_OFFSET	21
#define PT_OFFSET	12

#define MASK(x)		((1ul << (x)) - 1)
#define PAGEADDR(x)	((u64)(x) & ~PGMASK)
#define PAGEKADDR(x)	(PAGEADDR(x) | KERNBASE)
#define PA2PGINFO(x)	((struct page_info *)(k_pageinfo + ((u64)(x) >> 12)))
#define KA2PGINFO(x)	((struct page_info *)(k_pageinfo + (K2P(x) >> 12)))

#define PML4_INDEX(x)	(((u64)(x) >> PML4_OFFSET) & 511)
#define PDPT_INDEX(x)	(((u64)(x) >> PDPT_OFFSET) & 511)
#define PD_INDEX(x)	(((u64)(x) >> PD_OFFSET) & 511)
#define PT_INDEX(x)	(((u64)(x) >> PT_OFFSET) & 511)

#define ROUNDDOWN(x, blk_size)	(((x) / (blk_size)) * (blk_size))
#define ROUNDUP(x, blk_size)	((((x)+(blk_size)-1) / (blk_size)) * (blk_size))

struct page_info {
	u64			paddr;
	i64 			ref;
	struct page_info	*next;
};

struct mem_map_desc {
	u32	type;           // Field size is 32 bits followed by 32 bit pad
	u32	pad;
	u64	phys_start;  // Field size is 64 bits
	u64	virt_start;   // Field size is 64 bits
	u64	num_of_pages;  // Field size is 64 bits
	u64	attr;      // Field size is 64 bits
};

struct mem_map {
	u8	*list;
	u64	map_size,
		map_key,
		desc_size,
		desc_ver;
};

#define KERNBASE	0xFFFF800000000000
#define USTACK		0x00007FFFFFFFF000
#define USTACKTOP	0x00007FFFFFFFE000
#define KSTACK		0xFFFFFFFFFFFFF000
#define KSTACKTOP	0xFFFFFFFFFFFFE000
#define UARGS		0x3FF000
#define EXEC_IMG	0xFFFF900000000000
#define KMMIO		0xFFFFA00000000000
#define KDISK		0xFFFFC00000000000

#define K2P(x)		((u64)(x) & MASK(47))
#define P2K(x)		((u64)(x) | KERNBASE)

#define KERN_CODE_SEL	0x08
#define KERN_DATA_SEL	0x10
#define USER_CODE_SEL	0x18
#define USER_DATA_SEL	0x20

#define FLAG_ZERO	0x1

#define FREE_PGTBL_DECREF	0x1

#define CPY_PGTBL_CNTREF	0x1
#define CPY_PGTBL_NOKSPACE	0x2

/*

ukern uses 4-level paging. The addresses above KERNBASE is saved for
kernel. Other addresses can be used freely by user process.

############################# Virtual Memory Space #############################

MEM_MAX  ---------------------------------------------	0xFFFF FFFF FFFF FFFF
	^
	|				KSTACK		0xFFFF FFFF FFFF F000
	|				KSTACKTOP	0xFFFF FFFF FFFF E000
	|
	Kernel space, shared among kernel
	and all user process's
	|				KDISK		0xFFFF C000 0000 0000
	|				KMMIO		0xFFFF A000 0000 0000
	|				EXEC_IMG	0xFFFF 9000 0000 0000
	v
KERNBASE ---------------------------------------------	0xFFFF 8000 0000 0000
	^
	|				USTACK		0x0000 FFFF FFFF F000
	|				USTACKTOP	0x0000 FFFF FFFF E000
	|
	User space (in user process's pgtbl)
	/ Identically mapped (in k_pgtbl)
	|
	|				UTEXT		0x0000 0000 0040 0000
	|				UARGS		0x0000 0000 003F F000
	v
MEM_MIN  ---------------------------------------------	0x0000 0000 0000 0000

############################ Physical Memory Layout ############################

We know nothing about physical memory allocation, we get the info from ACPI.

*/

#define CR0_WP		(1ul << 16)

#define CR4_SMAP	(1ul << 21)
#define CR4_PKE		(1ul << 22)
#define CR4_PKS		(1ul << 24)

struct seg_desc {
	u16	limit1;
	u16	base1;
	u8	base2;
	u8	type:4, s:1, dpl:2, p:1;
	u8	limit2:4, avl:1, l:1, d_b:1, g:1;
	u8	base3;
};

extern struct seg_desc	gdt[9];

struct gdt_desc {
	u16 limit;
	u64 base;
} __attribute__((packed));

extern struct gdt_desc	gdt_desc;

struct tss {
	u32	res0;

	u64	rsp0;
	u64	rsp1;
	u64	rsp2;

	u64	res1;

	u64	ist1;
	u64	ist2;
	u64	ist3;
	u64	ist4;
	u64	ist5;
	u64	ist6;
	u64	ist7;

	u64	res2;
	u16	res3;
	u16	io_base;
} __attribute__((packed));

extern struct tss	tss;

struct tss_desc {
	u16	limit1;
	u16	base1;
	u8	base2;
	u8	type:4, s:1, dpl:2, p:1;
	u8	limit2:4, avl:1, l:1, d_b:1, g:1;
	u8	base3;
	u32	base4;
	u32	res;
} __attribute__((packed));

extern pgtbl_t		k_pgtbl;
extern struct page_info	*k_pageinfo;
extern char		*end_kmem;

void init_kpageinfo(struct mem_map *mem_map);
void init_kpgtbl(void);
void init_freepages(void);
int walk_pgtbl(pgtbl_t pgtbl, u64 vaddr, pte_t **pte, int create);
int map_mmio(pgtbl_t pgtbl, u64 vaddr, u64 mmioaddr, pte_t **pte);
struct page_info *alloc_page(u64 flags);
void free_page(struct page_info *page);
void init_gdt(void);
void free_pgtbl(pgtbl_t pgtbl, u64 flags);
int copy_pgtbl(pgtbl_t dst, pgtbl_t src, u64 flags);
void pgtbl_clearflags(pgtbl_t pgtbl, u64 flags);

#endif