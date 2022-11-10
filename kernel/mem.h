/*
 This file declares macros and functions used for memory management.
*/

#ifndef MEM_H
#define MEM_H

#include "types.h"

#define PGSIZE 4096
#define PML4_LEN 512
#define PDPT_LEN 512
#define PD_LEN 512
#define PT_LEN 512

#define PML4E_P 0x1
#define PML4E_W 0x2
#define PML4E_U 0x4
#define PDPTE_P 0x1
#define PDPTE_W 0x2
#define PDPTE_U 0x4
#define PDPTE_PS 0x80
#define PDE_P 0x1
#define PDE_W 0x2
#define PDE_U 0x4
#define PTE_P 0x1
#define PTE_W 0x2
#define PTE_U 0x4
#define PTE_FLAGS 0x7
#define PTE_PWT 0x8
#define PTE_PCD 0x10

#define PML4_OFFSET 39
#define PDPT_OFFSET 30
#define PD_OFFSET 21
#define PT_OFFSET 12

#define ADDR_MASK ~511
#define MASK(x) ((1ul << (x)) - 1)
#define PAGEADDR(x) ((x) & ~(PGSIZE - 1))
#define PAGEKADDR(x) (((x) & ~(PGSIZE - 1)) | KERNBASE)
#define PA2PGINFO(x) ((struct PageInfo *)(k_pageinfo + ((uint64_t)(x) >> 12)))
#define KA2PGINFO(x) ((struct PageInfo *)(k_pageinfo + (((uint64_t)(x) & MASK(47)) >> 12)))

#define PML4_INDEX(x) (((x) >> PML4_OFFSET) & 511)
#define PDPT_INDEX(x) (((x) >> PDPT_OFFSET) & 511)
#define PD_INDEX(x) (((x) >> PD_OFFSET) & 511)
#define PT_INDEX(x) (((x) >> PT_OFFSET) & 511)

#define ROUNDDOWN(x, blk_size) (((x) / (blk_size)) * (blk_size))
#define ROUNDUP(x, blk_size) ((((x) + (blk_size) - 1) / (blk_size)) * (blk_size))

struct PageInfo {
    uint64_t paddr;
    union {
        struct PageInfo *next;
        uint64_t ref;
    } u;
};

struct MemInfo {
    uint64_t paddr;
    uint64_t length;
    uint32_t type;
    uint32_t ext_attr; // we ignore it
};

#define KERNBASE 0xFFFF800000000000
#define USTACK   0x00007FFFFFFFF000
#define KSTACK   0xFFFF800000010000
#define K2P(x) ((uint64_t)(x) & MASK(47))
#define P2K(x) ((uint64_t)(x) | KERNBASE)

#define KERN_CODE_SEL 0x08
#define KERN_DATA_SEL 0x10
#define USER_CODE_SEL 0x18
#define USER_DATA_SEL 0x20

#define FLAG_ZERO 0x1

#define FREE_PGTBL_DECREF 0x1

#define CPY_PGTBL_CNTREF 0x1
#define CPY_PGTBL_WITHKSPACE 0x2

/*

ukern uses 4-level paging. The addresses above 0xFF0000000000 is saved for
kernel. Other addresses can be used freely by user process.

############################# Virtual Memory Space #############################

MEM_MAX  ---------------------------------------- 0xFFFFFFFFFFFF

KERNBASE ---------------------------------------- 0xFF8000000000  R/W, S

MEM_MIN  ---------------------------------------- 0x000000000000  R/W, U

############################ Physical Memory Layout ############################

PHYS_MAX ---------------------------------------- ?
        These address should can all be used.
EXT_16MB ---------------------------------------- 0x000001000000
        We hope these addresses can all be used.
EXT_1MB  ---------------------------------------- 0x000000100000
        We don't use these addresses after kernel is ready.
BASE     ---------------------------------------- 0x000000000000

*/

struct SegDesc {
    uint16_t limit1;
    uint16_t base1;
    uint8_t base2;
    uint8_t type    : 4,
            s       : 1,
            dpl     : 2,
            p       : 1;
    uint8_t limit2  : 4,
            avl     : 1,
            l       : 1,
            d_b     : 1,
            g       : 1;
    uint8_t base3;
};

extern struct SegDesc gdt[9];

struct GdtDesc {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

extern struct GdtDesc gdt_desc;

struct TSS {
    uint32_t res0;

    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;

    uint64_t res1;

    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;

    uint64_t res2;
    uint16_t res3;
    uint16_t io_base;
} __attribute__((packed));
extern struct TSS tss;

struct TSSDesc {
    uint16_t limit1;
    uint16_t base1;
    uint8_t base2;
    uint8_t type    : 4,
            s       : 1,
            dpl     : 2,
            p       : 1;
    uint8_t limit2  : 4,
            avl     : 1,
            l       : 1,
            d_b     : 1,
            g       : 1;
    uint8_t base3;
    uint32_t base4;
    uint32_t res;
} __attribute__((packed));

extern char end[];
extern pgtbl_t k_pml4;
extern uint64_t nfreepages;
extern char *end_kmem;
extern struct PageInfo *k_pageinfo;

void init_kpageinfo(void);
void init_kpgtbl(void);
void init_freepages(void);
int walk_pgtbl(pgtbl_t pgtbl, uint64_t vaddr, pte_t **pte, int create);
struct PageInfo *alloc_page(uint64_t flags);
void memcpy(char *dst, char *src, uint64_t n_bytes);
void init_gdt(void);
void free_pgtbl(pgtbl_t pgtbl, uint64_t flags);
int copy_pgtbl(pgtbl_t dst, pgtbl_t src, uint64_t flags);
void pgtbl_clearflags(pgtbl_t pgtbl, uint64_t flags);

#endif