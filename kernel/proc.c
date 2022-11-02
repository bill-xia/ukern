#include "proc.h"
#include "mem.h"
#include "elf64.h"
#include "x86.h"
#include "printk.h"
#include "sched.h"
#include "errno.h"

struct Proc *procs, *curproc;

void
init_pcb(void)
{
    procs = (struct Proc *)ROUNDUP((uint64_t)end_kmem, sizeof(struct Proc));
    end_kmem = (char *)(procs + NPROCS);
}

struct Proc *
alloc_proc(void)
{
    for (int i = 0; i < NPROCS; ++i) {
        if (procs[i].state == CLOSE) {
            // TODO: add lock here when multi-core is up
            struct PageInfo *page = alloc_page(FLAG_ZERO);
            procs[i].pgtbl = (pgtbl_t)page->paddr;
            page = alloc_page(FLAG_ZERO);
            procs[i].p_pgtbl = (pgtbl_t)page->paddr;
            procs[i].state = PENDING;
            procs[i].pid = i + 1;
            return procs + i;
        }
    }
    return NULL;
}

uint64_t min(uint64_t a, uint64_t b) {
    return a < b ? a : b;
}

int
create_proc(char *img)
{
    int ret = 0;
    // printk("nfreepages before create_proc(): %d\n", nfreepages);
    struct Proc *proc = alloc_proc();
    if (proc == NULL) {
        return -E_NOPCB;
    }
    if (ret = load_img(img, proc)) {
        return ret;
    }
    // stack
    pte_t *pte;
    if (ret = walk_pgtbl(proc->pgtbl, USTACK - PGSIZE, &pte, 1)) {
        free_pgtbl(proc->pgtbl, FREE_PGTBL_DECREF);
        return ret;
    }
    *pte |= PTE_U | PTE_W;
    // mapping kernel space
    for (int i = 256; i < 512; ++i) {
        proc->pgtbl[i] = k_pml4[i];
    }
    // set up runtime enviroment
    struct Elf64_Ehdr *ehdr = (struct Elf64_Ehdr *)img;
    proc->context.rip = ehdr->e_entry;
    proc->context.cs = USER_CODE_SEL | 3;
    proc->context.rsp = USTACK;
    proc->context.ss = USER_DATA_SEL | 3;
    proc->context.rflags = 0x02;
    proc->state = READY;
    // printk("nfreepages after create_proc(): %d\n", nfreepages);
}

void
run_proc(struct Proc *proc)
{
    curproc = proc;
    struct ProcContext *context = &proc->context;
    lcr3((uint64_t)proc->pgtbl);
    asm volatile (
        "mov %0, %%rsp\n"
        "popq %%rax\n"
        "popq %%rcx\n"
        "popq %%rdx\n"
        "popq %%rbx\n"
        "popq %%rdi\n"
        "popq %%rsi\n"
        "popq %%rbp\n"
        "popq %%r8\n"
        "popq %%r8\n"
        "popq %%r9\n"
        "popq %%r10\n"
        "popq %%r11\n"
        "popq %%r12\n"
        "popq %%r13\n"
        "popq %%r14\n"
        "popq %%r15\n"
        "add $16, %%rsp\n"
        "iretq \n"
        :: "g"(context) : "memory");
}

void
kill_proc(struct Proc *proc)
{
    // printk("nfreepages before kill_proc(): %d\n", nfreepages);
    proc->state = PENDING;
    free_pgtbl((pgtbl_t)P2K(proc->pgtbl), FREE_PGTBL_DECREF);
    free_pgtbl((pgtbl_t)P2K(proc->p_pgtbl), 0);
    proc->state = CLOSE;
    // printk("nfreepages after kill_proc(): %d\n", nfreepages);
}

//
// Load program image pointed by `ehdr`
// into `proc`'s memory space.
// Returns 0 on success,
//        -ENOMEM when memory not enough,
//        -EFORMAT when img is not ELF64 file.
//
int
load_img(char *img, struct Proc *proc)
{
    struct Elf64_Ehdr *ehdr = (struct Elf64_Ehdr *)img;
    if (*(uint32_t *)ehdr->e_ident != ELF_MAGIC) {
        return -E_FORMAT;
    }
    struct Elf64_Phdr *phdr = (struct Elf64_Phdr *)(img + ehdr->e_phoff);
    for (int i = 0; i < ehdr->e_phnum; ++i, ++phdr) {
        if (phdr->p_type != PT_LOAD)
            continue;
        char *src = img + phdr->p_offset, 
             *src_fileend = src + phdr->p_filesz,
             *src_memend = src + phdr->p_memsz;
        uint64_t vaddr = phdr->p_vaddr;
        while (src < src_memend) {
            pte_t *pte;
            int ret = walk_pgtbl(proc->pgtbl, PAGEADDR(vaddr), &pte, 1);
            if (ret) {
                free_pgtbl(proc->pgtbl, FREE_PGTBL_DECREF);
                return -E_NOMEM;
            }
            *pte |= PTE_U | PTE_W;
            if (src <= src_fileend) {
                uint64_t siz = min(PGSIZE - (vaddr & (PGSIZE - 1)), (src_fileend - src));
                memcpy((char *)(PAGEADDR(*pte) + (vaddr & (PGSIZE - 1))), src, siz);
            }
            src = (char *)PAGEADDR((uint64_t)src + PGSIZE);
            vaddr = PAGEADDR(vaddr + PGSIZE);
        }
    }
    return 0;
}