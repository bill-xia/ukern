#include "stdio.h"
#include "usyscall.h"

int main()
{
    printf("before fork\n");
    int child;
    if (child = sys_fork()) {
        printf("parent after fork, child: %d\n", child);
    } else {
        printf("child after fork\n");
        // while(1);
    }
    sys_exit();
}