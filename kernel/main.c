#include "init.h"
#include "printk.h"
#include "intr.h"
#include "proc.h"
#include "sched.h"

int main()
{
    printk("\"hello, world!\" from ukern!\n");
    init();
    printk("bye\n");
    CREATE_PROC(hello);
    sched();
    while (1);
}