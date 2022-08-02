/*
 This file declares macros and functions used for memory management.
*/

#ifndef MEM_H
#define MEM_H

#define MEM_MIN 0x000000000000
#define MEM_MAX 0xFFFFFFFFFFFF
#define KERNBASE 0xFF0000000000

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
#define PDE_P 0x1
#define PDE_W 0x2
#define PTE_P 0x1
#define PTE_W 0x2

#define PML4_ADDR_MASK (~(0xFFFul))
#define PDPT_ADDR_MASK (~(0xFFFul))
#define PD_ADDR_MASK (~(0xFFFul))
#define PT_ADDR_MASK (~(0xFFFul))

#define PML4_OFFSET 39
#define PDPT_OFFSET 30
#define PD_OFFSET 21
#define PT_OFFSET 12

#define PML4_INDEX(x) ((x >> PML4_OFFSET) & 512)
#define PDPT_INDEX(x) ((x >> PDPT_OFFSET) & 512)
#define PD_INDEX(x) ((x >> PD_OFFSET) & 512)
#define PT_INDEX(x) ((x >> PT_OFFSET) & 512)

#define FLOOR(x, blk_size) (((x) / (blk_size)) * (blk_size))
#define CEIL(x, blk_size) ((((x) + (blk_size) - 1) / (blk_size)) * (blk_size))

struct PageInfo {
    uint64_t paddr;
    uint16_t ref;
};

struct MemInfo {
    uint64_t paddr;
    uint64_t length;
    uint32_t type;
    uint32_t ext_attr; // we ignore it
};

/*

ukern uses 4-level paging. The addresses above 0xFF0000000000 is saved for
kernel. Other addresses can be used freely by user process.

############################# Virtual Memory Space #############################

MEM_MAX  ---------------------------------------- 0xFFFFFFFFFFFF

KERNBASE ---------------------------------------- 0xFF0000000000  R/W, S

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

void mem_init();
uint64_t *walk_pgtbl();

#endif