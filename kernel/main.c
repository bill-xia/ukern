#include "init.h"
#include "printk.h"
#include "intr.h"
#include "proc.h"
#include "sched.h"

int main()
{
    init();
    CREATE_PROC(hello);
    CREATE_PROC(sort);
    CREATE_PROC(divzero);
    CREATE_PROC(fork);
    sched();
    while (1);
}