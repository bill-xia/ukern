#include "syscall.h"
#include "printk.h"
#include "console.h"
#include "proc.h"
#include "mem.h"
#include "x86.h"
#include "sched.h"
#include "ide.h"
#include "fs.h"

void
sys_hello(void)
{
    printk("\"hello, world!\" from user space!\n");
}

void
sys_fork(struct ProcContext *tf)
{
    struct Proc *nproc = alloc_proc();
    copy_pgtbl(nproc->pgtbl, curproc->pgtbl, CPY_PGTBL_CNTREF | CPY_PGTBL_WITHKSPACE);
    // save the "real" page table
    copy_pgtbl(nproc->p_pgtbl, curproc->pgtbl, 0);
    free_pgtbl(curproc->p_pgtbl, 0);
    curproc->p_pgtbl = (pgtbl_t)alloc_page(FLAG_ZERO)->paddr;
    copy_pgtbl(curproc->p_pgtbl, curproc->pgtbl, 0);
    // clear write flag at "write-on-copy" pages
    pgtbl_clearflags(nproc->pgtbl, PTE_W);
    pgtbl_clearflags(curproc->pgtbl, PTE_W);
    // copy context
    nproc->context = *tf;
    // set return value
    tf->rax = nproc->pid;
    nproc->context.rax = 0;
    // set state
    nproc->state = READY;
    lcr3(rcr3());
}

uint8_t to_upper_case(uint8_t c)
{
    if ('a' <= c && c <= 'z') c += 'A' - 'a';
    return c;
}

int cmp_fn(uint8_t c1, uint16_t c2) {
    printk("cmp_fn(%c,%c):%x  ",c1,c2,to_upper_case(c1) != to_upper_case(c2));
    return to_upper_case(c1) != to_upper_case(c2);
}

void
sys_open(struct ProcContext *tf)
{
    uint32_t head_cluster;
    int r = open_file((void *)(tf->rdx), &head_cluster);
    if (r == 0) {
        for (int fd = 0; fd < 256; ++fd) {
            if (curproc->fdesc[fd].head_cluster) continue;
            curproc->fdesc[fd].head_cluster = head_cluster;
            tf->rax = fd;
            return;
        }
        // no available file descriptor
        tf->rax = -E_NO_AVAIL_FD;
        return;
    } else {
        tf->rax = r;
        return;
    }
}

void
sys_read(struct ProcContext *tf)
{
    printk("sys_read() not implemented yet.\n");
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
    case 5:
        sys_open(tf);
        break;
    case 6:
        sys_read(tf);
        break;
    default:
        printk("unknown syscall\n");
        while (1);
    }
}