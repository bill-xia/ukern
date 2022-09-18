#include "init.h"
#include "ukernio.h"
#include "intr.h"

int main()
{
    printk("\"hello, world!\" from ukern!\n");
    init();
    printk("bye\n");
    int a = 1 / 0;
    while (1);
}

void __stack_chk_fail (void)
{
    //
}