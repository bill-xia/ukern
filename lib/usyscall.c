#include "types.h"

int
syscall(int num, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6)
{
    int ret;
    asm volatile (
        "int $32"
        :"=a" (ret):
        "a" (num),
        "d" (a1),
        "c" (a2),
        "b" (a3),
        "D" (a4),
        "S" (a5): "cc", "memory"
    );
    return ret;
}

void
sys_hello(void)
{
    syscall(2, 0, 0, 0, 0, 0, 0);
}

void
sys_putch(char c)
{
    syscall(3, c, 0,0,0,0,0);
}

void
sys_exit(void)
{
    syscall(1,0,0,0,0,0,0);
}

int
sys_fork(void)
{
    return syscall(4,0,0,0,0,0,0);
}

int
sys_open(const char *fn)
{
    return syscall(5, fn, 0,0,0,0,0);
}

int
sys_read(void)
{
    return syscall(6, 0, 0,0,0,0,0);
}