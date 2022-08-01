#include "mem.h"
#include "ukernio.h"

int main()
{
    print("\"hello, world!\" from ukern!");
    newline();
    init_mem();
    while (1);
}

void __stack_chk_fail (void)
{
    //
}