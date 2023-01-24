#include "syscall.h"
#include "printk.h"
#include "console.h"
#include "proc.h"
#include "mem.h"
#include "elf64.h"
#include "x86.h"
#include "sched.h"
#include "fs/fs.h"
#include "kbd.h"

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
    copy_pgtbl(nproc->pgtbl, curproc->p_pgtbl, CPY_PGTBL_CNTREF | CPY_PGTBL_WITHKSPACE);
    // save the "real" page table
    copy_pgtbl(nproc->p_pgtbl, curproc->p_pgtbl, CPY_PGTBL_WITHKSPACE);
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
    uint32_t head_cluster, use_fat;
    uint64_t file_len;
    int r = open_file((void *)(tf->rdx), &head_cluster, &file_len, &use_fat);
    if (r == 0) {
        for (int fd = 0; fd < 256; ++fd) {
            if (curproc->fdesc[fd].head_cluster) continue;
            curproc->fdesc[fd].head_cluster = head_cluster;
            curproc->fdesc[fd].read_ptr = 0;
            curproc->fdesc[fd].file_len = file_len;
            curproc->fdesc[fd].use_fat = use_fat;
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
        min(tf->rbx, fdesc->file_len - fdesc->read_ptr),
        fdesc->use_fat);
    tf->rax = r;
    curproc->fdesc[fd].read_ptr += r;
}

void strncpy(char *dst, char *src, int n)
{
    for (int i = 0; i < n && *src != '\0'; ++i) {
        *dst = *src;
        src++;
        dst++;
    }
}

void
sys_exec(struct ProcContext *tf)
{
    static char argv_buf[16][256];
    uint32_t head_cluster, use_fat;
    uint64_t file_len;
    int ret = 0;
    int r = open_file((void *)(tf->rdx), &head_cluster, &file_len, &use_fat);
    if (r) { // open file failed
        tf->rax = r;
        return;
    }
    // save argv
    char **ori_argv = (char **)tf->rbx;
    for (int i = 0; i < NARGS; ++i) {
        if (ori_argv[i] != NULL) {
            strncpy(argv_buf[i], ori_argv[i], ARGLEN);
        }
    }
    // read image
    char *img = (char *)EXEC_IMG;
    uint64_t addr = EXEC_IMG, addr_end = EXEC_IMG + file_len;
    lcr3(K2P(k_pgtbl));
    // if (img < img_end) {
        // file too large, seldom happen
    // }
    while (addr < addr_end) {
        pte_t *pte;
        if (ret = walk_pgtbl(k_pgtbl, addr, &pte, 1)) {
            tf->rax = -E_NOMEM;
            // TODO: free memory for img?
            return;
        }
        *pte |= PTE_U | PTE_W;
        addr += PGSIZE;
    }
    lcr3(rcr3());
    read_file(head_cluster, 0, img, file_len, use_fat);
    // TODO: check file format before destroying current enviroment,
    // return enough information if format error
    curproc->state = PENDING;
    // from now on, old process image is destroyed
    free_pgtbl(curproc->pgtbl, FREE_PGTBL_DECREF);
    free_pgtbl(curproc->p_pgtbl, 0);
    struct PageInfo *page = alloc_page(FLAG_ZERO);
    curproc->pgtbl = (pgtbl_t)page->paddr;
    page = alloc_page(FLAG_ZERO);
    curproc->p_pgtbl = (pgtbl_t)page->paddr;
    // code below is copied from create_proc()
    // may delete create_proc() in the future
    struct Proc *proc = curproc;
    if (ret = load_img(img, proc)) {
        kill_proc(proc);
        sched();
    }
    // TODO: unmap img here
    // stack
    pte_t *pte;
    if (ret = walk_pgtbl(proc->pgtbl, USTACK - PGSIZE, &pte, 1)) {
        kill_proc(proc);
        sched();
    }
    *pte |= PTE_U | PTE_W;
    // uargs
    char **uargv = (char **)(PAGEKADDR(*pte) + PGSIZE - NARGS * sizeof(uint64_t));
    for (int i = 0; i < NARGS; ++i) {
        uargv[i] = (char *)((uint64_t)UARGS + 256 * i);
    }
    if (ret = walk_pgtbl(proc->pgtbl, UARGS, &pte, 1)) {
        kill_proc(proc);
        sched();
    }
    *pte |= PTE_U | PTE_W;
    char *uarg_pg = (char *)PAGEKADDR(*pte);
    for (int i = 0; i < NARGS; ++i) {
        strncpy(uarg_pg + i * ARGLEN, argv_buf[i], ARGLEN);
    }
    // mapping kernel space
    pgtbl_t pgtbl = (pgtbl_t)P2K(proc->pgtbl);
    for (int i = 256; i < 512; ++i) {
        pgtbl[i] = k_pgtbl[i];
    }
    copy_pgtbl(proc->p_pgtbl, proc->pgtbl, CPY_PGTBL_WITHKSPACE);
    // set up runtime enviroment
    struct Elf64_Ehdr *ehdr = (struct Elf64_Ehdr *)img;
    proc->context.rip = ehdr->e_entry;
    proc->context.cs = USER_CODE_SEL | 3;
    proc->context.rsp = USTACK - NARGS * sizeof(uint64_t);
    proc->context.ss = USER_DATA_SEL | 3;
    proc->context.rflags = 0x02 | RFLAGS_IF | (3 << RFLAGS_IOPL_SHIFT);
    // argc in rdi
    proc->context.rdi = tf->rcx;
    // argv in rsi, which is predefined
    proc->context.rsi = USTACK - NARGS * sizeof(uint64_t);
    proc->state = READY;
    sched();
}

#define E_KBD_IN_USE 1
void
sys_getch(struct ProcContext *tf)
{
    if (kbd_buf_siz > 0) {
        tf->rax = kbd_buffer[kbd_buf_beg++];
        if (kbd_buf_beg == 4096)
            kbd_buf_beg = 0;
        kbd_buf_siz--;
        return;
    }
    if (kbd_proc == NULL) {
        kbd_proc = curproc;
    } else {
        tf->rax = -E_KBD_IN_USE; // keyboard in use
        return;
    }
    curproc->context = *tf;
    curproc->state = PENDING;
    sched();
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
    case 7:
        sys_exec(tf);
        break;
    case 8:
        sys_getch(tf);
        break;
    default:
        printk("unknown syscall\n");
        while (1);
    }
}