#include "mem.h"
#include "proc.h"
#include "intr.h"

void init()
{
    end_kmem = end;
    init_kpageinfo(); // after the kernel image, is the k_pageinfo array
    init_kpgtbl(); // then comes kernel pagetable, which maps the whole physical memory space
    init_proc(); // then Process Control Blocks
    init_intr();
    // maybe some TSS/LDT struct here
    // init_freepages(); // initialize the free page list
    // from now on, anyone needing a new page have to call kalloc()
}