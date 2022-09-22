#include "init.h"
#include "ukernio.h"
#include "intr.h"

int main()
{
    printk("\"hello, world!\" from ukern!\n");
    init();
    printk("bye\n");
    while (1);
}