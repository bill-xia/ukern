#include "sched.h"
#include "proc.h"
#include "printk.h"

void
sched(void) // never returns
{
    while (1) {
        for (int i = 0; i < NPROCS; ++i) {
            if (procs[i].state == READY) {
                procs[i].state = RUNNING;
                run_proc(procs + i);
            }
        }
        printk("no proc to sched\n");
        while (1);
    }
}