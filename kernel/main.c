#include "mem.h"
#include "ukernio.h"

int main()
{
    printf("\"hello, world!\" from ukern!\n");
    init_mem();
    while (1);
}

void __stack_chk_fail (void)
{
    //
}