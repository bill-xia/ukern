#include "proc.h"
#include "mem.h"
#include "elf64.h"
#include "x86.h"
#include "ukernio.h"
// struct TSS g_tss;

void
init_pcb(void)
{
    procs = (struct Proc *)ROUNDUP((uint64_t)end_kmem, sizeof(struct Proc));
    end_kmem = (char *)(procs + NPROCS);
}

struct Proc *
alloc_proc()
{
    for (int i = 0; i < NPROCS; ++i) {
        if (procs[i].state == CLOSE) {
            // TODO: add lock here when multi-core is up
            struct PageInfo *page = alloc_page(FLAG_ZERO);
            procs[i].pgtbl = page->paddr;
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
    struct Proc *proc = alloc_proc();
    if (proc == NULL) {
        printk("no pcb available\n");
        while (1);
    }
    struct Elf64_Ehdr *ehdr = (struct Elf64_Ehdr *)img;
    if (*(uint32_t *)ehdr->e_ident != ELF_MAGIC) {
        printk("not elf64 file\n");
        while (1);
    }
    struct Elf64_Phdr *phdr = (struct Elf64_Phdr *)(img + ehdr->e_phoff);
    for (int i = 0; i < ehdr->e_phnum; ++i, ++phdr) {
        char *src = img + phdr->p_offset, 
             *src_fileend = src + phdr->p_filesz,
             *src_memend = src + phdr->p_memsz;
        uint64_t vaddr = phdr->p_vaddr;
        while (src < src_memend) {
            uint64_t *pte;
            int ret = walk_pgtbl((void *)proc->pgtbl, PAGEADDR(vaddr), &pte, 1);
            if (ret) {
                printk("no mem\n");
                while (1);
            }
            *pte |= PTE_U | PTE_W;
            if (src <= src_fileend) {
                uint64_t siz = min(PGSIZE - (vaddr & (PGSIZE - 1)), (src_fileend - src));
                memcpy((void *)(PAGEADDR(*pte) + (vaddr & (PGSIZE - 1))), src, siz);
            }
            src = (char *)PAGEADDR((uint64_t)src + PGSIZE);
            vaddr = PAGEADDR(vaddr + PGSIZE);
        }
    }
    uint64_t *pte;
    walk_pgtbl((void *)proc->pgtbl, USTACK - PGSIZE, &pte, 1);
    *pte |= PTE_U | PTE_W;
    extern uint64_t *k_pml4;
    for (int i = 256; i < 512; ++i) {
        ((uint64_t *)(proc->pgtbl))[i] = k_pml4[i];
    }
    proc->rip = ehdr->e_entry;
    proc->cs = USER_CODE_SEL | 3;
    proc->rsp = USTACK;
    proc->ss = USER_DATA_SEL | 3;
    proc->rflags = 0x02;
    lcr3(proc->pgtbl);
    restore_context(&proc->context);
}

void
restore_context(struct ProcContext *context)
{
    // while (1);
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
        "iretq \n"
        :: "g"(context) : "memory");
}