#include "mem.h"

void init()
{
    end_kmem = end;
    init_kpageinfo(); // after the kernel image, is the k_pageinfo array
    init_kpgtbl(); // then comes kernel pagetable, which maps the whole physical memory space
    // init_pcb(); // then Process Control Blocks
    // maybe some TSS/LDT struct here
    // init_freepages(); // initialize the free page list
    // from now on, anyone needing a new page have to call kalloc()
}