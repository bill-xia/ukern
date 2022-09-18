#include "proc.h"
#include "mem.h"

void
init_proc(void)
{
    procs = (struct Proc *)ROUNDUP((uint64_t)end_kmem, sizeof(struct Proc));
    end_kmem = procs + NPROCS;
}