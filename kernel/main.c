#include "init.h"
#include "ukernio.h"

int main()
{
    printk("\"hello, world!\" from ukern!\n");
    init();
    printk("bye\n");
    while (1);
}

void __stack_chk_fail (void)
{
    //
}