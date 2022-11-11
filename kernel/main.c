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
    printk("buf: %p\n", K2P(buf));
    ide_read(0, (void *)K2P((uint64_t)buf), 1);
    printk("done\n");
    for (int i = 0; i < 16; ++i) {
        printk("%c", buf[i]);
    }
    CREATE_PROC(hello);
    CREATE_PROC(sort);
    CREATE_PROC(divzero);
    CREATE_PROC(fork);
    CREATE_PROC(idle);
    sched();
    while (1);
}