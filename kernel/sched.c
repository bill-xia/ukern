#include "sched.h"
#include "proc.h"
#include "printk.h"

void
sched(void) // never returns
{
    while (1) {
        int curid = curproc ? curproc - procs : 0;
        for (int i = (curid + 1) % NPROCS; i != curid; i = (i + 1 != NPROCS ? i + 1 : 0)) {
            if (procs[i].state == READY) {
                procs[i].state = RUNNING;
                run_proc(procs + i);
            }
        }
        // i == curid
        if (procs[curid].state == READY) {
            procs[curid].state = RUNNING;
            run_proc(procs + curid);
        }
        // printk("no proc to sched\n");
        // while (1);
    }
}