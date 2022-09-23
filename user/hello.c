#include "stdio.h"
#include "usyscall.h"

int main()
{
    printf("hello from user space!\n");
    sys_exit();
}