#include "init.h"
#include "printk.h"
#include "intr.h"
#include "proc.h"
#include "sched.h"
#include "errno.h"
#include "ide.h"
#include "mem.h"

char buf[512];

int main()
{
    init();
    // CREATE_PROC(hello);
    // CREATE_PROC(sort);
    // CREATE_PROC(divzero);
    // // CREATE_PROC(fork);
    CREATE_PROC(idle);
    // CREATE_PROC(read);
    // CREATE_PROC(mal_read);
    // // CREATE_PROC(exec);
    // CREATE_PROC(forkexec);
    CREATE_PROC(sh);
    sched();
    while (1);
}