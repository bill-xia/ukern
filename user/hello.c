#include "stdio.h"
#include "usyscall.h"

int main(int argc, char *argv[])
{
    printf("argc: %d\n", argc);
    for (int i = 0; i < argc; ++i) {
        printf("argv[%d]: ", i);
        printf(argv[i]);
        printf("\n");
    }
    printf("hello from user space!\n");
    sys_exit();
}