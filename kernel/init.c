#include "mem.h"
#include "proc.h"
#include "intr.h"
#include "sched.h"

void init()
{
    end_kmem = end;
    init_gdt();
    init_kpageinfo(); // after the kernel image, is the k_pageinfo array
    init_kpgtbl(); // then comes kernel pagetable, which maps the whole physical memory space
    init_pcb(); // then Process Control Blocks
    init_intr();
    init_freepages(); // initialize the free page list
    // *(int*)0x7FFFFFFF0000 = 0;
    // from now on, anyone needing a new page have to call kalloc()
}