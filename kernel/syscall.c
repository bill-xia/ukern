#include "syscall.h"
#include "printk.h"
#include "console.h"
#include "proc.h"
#include "mem.h"
#include "x86.h"
#include "sched.h"
#include "ide.h"
#include "fs.h"

int
check_umem_mapping(uint64_t addr, uint64_t siz)
{
    addr = PAGEADDR(addr);
    uint64_t end = PAGEADDR(addr + siz) + PGSIZE;
    uint64_t *pte, flags = PTE_U | PTE_P | PTE_W;
    pgtbl_t pgtbl = (pgtbl_t)P2K(rcr3());
    while (addr != end) {
        walk_pgtbl(pgtbl, addr, &pte, 0);
        if ((*pte & flags) != flags) {
            return 0;
        }
        addr += PGSIZE;
    }
    return 1;
}

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

void
sys_open(struct ProcContext *tf)
{
    uint32_t head_cluster;
    uint64_t file_len;
    int r = open_file((void *)(tf->rdx), &head_cluster, &file_len);
    if (r == 0) {
        for (int fd = 0; fd < 256; ++fd) {
            if (curproc->fdesc[fd].head_cluster) continue;
            curproc->fdesc[fd].head_cluster = head_cluster;
            curproc->fdesc[fd].read_ptr = 0;
            curproc->fdesc[fd].file_len = file_len;
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

inline uint64_t
min(uint64_t a, uint64_t b)
{
    return a < b ? a : b;
}

void
sys_read(struct ProcContext *tf)
{
    int fd = tf->rdx;
    if (fd < 0 || fd >= 64) {
        tf->rax = -E_INVALID_FD;
        return;
    }
    struct file_desc *fdesc = &curproc->fdesc[fd];
    if (fdesc->head_cluster == 0) {
        tf->rax = -E_FD_NOT_OPENED;
        return;
    }
    if (!check_umem_mapping(tf->rcx, tf->rbx)) {
        tf->rax = -E_INVALID_MEM;
        return;
    }
    int r = read_file(fdesc->head_cluster, 
        fdesc->read_ptr,
        (void *)tf->rcx,
        min(tf->rbx, fdesc->file_len - fdesc->read_ptr));
    tf->rax = r;
    curproc->fdesc[fd].read_ptr += r;
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