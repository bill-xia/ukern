#include "syscall.h"
#include "printk.h"
#include "console.h"
#include "proc.h"
#include "mem.h"
#include "x86.h"
#include "sched.h"

extern uint64_t *k_pml4;

void
sys_hello()
{
    printk("\"hello, world!\" from user space!\n");
}

void
sys_fork(struct ProcContext *tf)
{
    lcr3(k2p(k_pml4));
    struct Proc *nproc = alloc_proc();
    copy_uvm((void *)nproc->pgtbl, (void *)curproc->pgtbl, PTE_U);
    for (int i = 256; i < 512; ++i) {
        ((uint64_t *)nproc->pgtbl)[i] = ((uint64_t *)curproc->pgtbl)[i];
    }
    // copy context
    nproc->context = *tf;
    // set return value
    tf->rax = nproc->pid;
    nproc->context.rax = 0;
    // set state
    nproc->state = READY;
    lcr3(curproc->pgtbl);
}

void
syscall(struct ProcContext *tf)
{
    int num = tf->rax;
    switch (num) {
    case 1:
        curproc->state = PENDING;
        kill_proc(curproc);
        sched();
    case 2:
        sys_hello();
        tf->rax = 0;
        break;
    case 3:
        console_putch(tf->rdx);
        break;
    case 4:
        sys_fork(tf);
        break;
    default:
        printk("unknown syscall\n");
        while (1);
    }
}