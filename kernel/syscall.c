#include "syscall.h"
#include "printk.h"
#include "console.h"
#include "proc.h"

void
syscall(struct ProcContext *tf)
{
    int num = tf->rax;
    switch (num) {
    case 1:
        kill_proc(curproc);
    case 2:
        sys_hello();
        tf->rax = 0;
        break;
    case 3:
        console_putch(tf->rdx);
        break;
    default:
        printk("unknown syscall\n");
        while (1);
    }
}

void
sys_hello()
{
    printk("\"hello, world!\" from user space!\n");
}