#include "stdio.h"
#include "usyscall.h"

int main()
{
    for (int i = 1; i <= 100000000; ++i) ;
    printf("hello from user space!\n");
    int fd = sys_open("/fork");
    printf("opened file: %d\n", fd);
    fd = sys_open("/hello");
    printf("opened file: %d\n", fd);
    sys_exit();
}