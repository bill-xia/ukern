/*
 This file declares macros and functions used for memory management.
*/

#ifndef MEM_H
#define MEM_H

#include "types.h"

typedef uint64_t pml4e_t;
typedef uint64_t pdpte_t;
typedef uint64_t pde_t;
typedef uint64_t pte_t;

#define PGSIZE 4096
#define PML4_LEN 512
#define PDPT_LEN 512
#define PD_LEN 512
#define PT_LEN 512

#define PML4E_P 0x1
#define PML4E_W 0x2
#define PDPTE_P 0x1
#define PDPTE_W 0x2
#define PDPTE_PS 0x80
#define PDE_P 0x1
#define PDE_W 0x2
#define PTE_P 0x1
#define PTE_W 0x2

#define PML4_OFFSET 39
#define PDPT_OFFSET 30
#define PD_OFFSET 21
#define PT_OFFSET 12

#define ADDR_MASK ~511
#define MASK(x) ((1ul << x) - 1)

#define PML4_INDEX(x) ((x >> PML4_OFFSET) & 511)
#define PDPT_INDEX(x) ((x >> PDPT_OFFSET) & 511)
#define PD_INDEX(x) ((x >> PD_OFFSET) & 511)
#define PT_INDEX(x) ((x >> PT_OFFSET) & 511)

#define ROUNDDOWN(x, blk_size) (((x) / (blk_size)) * (blk_size))
#define ROUNDUP(x, blk_size) ((((x) + (blk_size) - 1) / (blk_size)) * (blk_size))

struct PageInfo {
    uint64_t paddr;
    union {
        struct PageInfo *next;
        uint64_t ref;
    } m;
};

struct MemInfo {
    uint64_t paddr;
    uint64_t length;
    uint32_t type;
    uint32_t ext_attr; // we ignore it
};

#define KERNBASE 0xFFFF800000000000
#define k2p(x) ((uint64_t)x & ((1ul << 47) - 1))

#define KERN_CODE_SEL 0x08
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

void init_kpageinfo();
void init_kpgtbl();
extern char end[];
char *end_kmem; // end of used kernel memory, in absolute address

#endif