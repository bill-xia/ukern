#include "mem.h"
#include "ukernio.h"

int main()
{
    clean();
    print("\"hello, world!\" from ukern!");
    newline();
    init_mem();
    while (1);
}

void __stack_chk_fail (void)
{
    //
}