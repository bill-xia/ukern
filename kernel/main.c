#include "init.h"
#include "printk.h"
#include "intr.h"
#include "proc.h"
#include "sched.h"

int main()
{
    for (int i = 0; i < 25; ++i) printk("\n");
    printk("\"hello, world!\" from ukern!\n");
    init();
    printk("bye\n");
    CREATE_PROC(hello);
    CREATE_PROC(sort);
    CREATE_PROC(divzero);
    CREATE_PROC(fork);
    sched();
    while (1);
}