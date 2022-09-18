#include "proc.h"
#include "mem.h"

// struct TSS g_tss;

void
init_proc(void)
{
    procs = (struct Proc *)ROUNDUP((uint64_t)end_kmem, sizeof(struct Proc));
    end_kmem = procs + NPROCS;
}