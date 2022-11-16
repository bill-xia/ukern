#include "stdio.h"
#include "usyscall.h"

char *argv[16] = {
    "hello",
    "arg"
};

int main()
{
    printf("before exec\n");
    sys_exec("/hello", 2, argv);
    printf("after exec\n");
    sys_exit();
}