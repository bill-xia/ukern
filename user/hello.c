#include "stdio.h"
#include "usyscall.h"

int main()
{
    for (int i = 1; i <= 1000000000; ++i) ;
    printf("hello from user space!\n");
    sys_exit();
}