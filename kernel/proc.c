#include "proc.h"
#include "mem.h"
#include "elf64.h"
#include "x86.h"
#include "printk.h"
#include "sched.h"
#include "errno.h"

struct Proc *procs, *curproc, *kbd_proc;
struct ProcContext empty_context;

void
clear_proc_context(struct ProcContext *context)
{
	memset(context, 0, sizeof(struct ProcContext));
}

void
init_pcb(void)
{
	procs = (struct Proc *)ROUNDUP((u64)end_kmem, sizeof(struct Proc));
	for (int i = 0; i < NPROCS; ++i) {
		procs[i].state = 0;
	}
	end_kmem = (char *)(procs + NPROCS);
}

struct Proc *
alloc_proc(void)
{
	struct Proc *proc;
	for (proc = procs; proc < procs + NPROCS; ++proc) {
		if (proc->state != CLOSE) {
			continue;
		}
		// TODO: add lock here when multi-core is up
		struct PageInfo *page = alloc_page(FLAG_ZERO);
		proc->pgtbl = (pgtbl_t)page->paddr;
		page = alloc_page(FLAG_ZERO);
		proc->p_pgtbl = (pgtbl_t)page->paddr;
		proc->state = PENDING;
		proc->pid = (proc - procs) + 1;
		proc->exec_time = 0;
		clear_proc_context(&proc->context);
		proc->parent = NULL;
		proc->next_sibling = NULL;
		proc->prev_sibling = NULL;
		proc->living_child = NULL;
		proc->zombie_child = NULL;
		return proc;
	}
	return NULL;
}

u64 min(u64 a, u64 b) {
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
	// uargs
	char **uargv = (char **)(PAGEKADDR(*pte) + PGSIZE - NARGS * sizeof(u64));
	for (int i = 0; i < NARGS; ++i) {
		uargv[i] = (char *)((u64)UARGS + 256 * i);
	}
	if (ret = walk_pgtbl(proc->pgtbl, UARGS, &pte, 1)) {
		free_pgtbl(proc->pgtbl, FREE_PGTBL_DECREF);
		return ret;
	}
	*pte |= PTE_U | PTE_W;
	// mapping kernel space
	for (int i = 256; i < 512; ++i) {
		proc->pgtbl[i] = k_pgtbl[i];
	}
	copy_pgtbl(proc->p_pgtbl, proc->pgtbl, CPY_PGTBL_WITHKSPACE);
	// set up runtime enviroment
	struct Elf64_Ehdr *ehdr = (struct Elf64_Ehdr *)img;
	proc->context.rip = ehdr->e_entry;
	proc->context.cs = USER_CODE_SEL | 3;
	proc->context.rsp = USTACK - NARGS * sizeof(u64);
	proc->context.ss = USER_DATA_SEL | 3;
	proc->context.rflags = 0x02 | RFLAGS_IF | (3 << RFLAGS_IOPL_SHIFT);
	// argc in rdi, which should be find elsewhere
	// argv in rsi, which is predefined
	proc->context.rsi = USTACK - NARGS * sizeof(u64);
	proc->state = READY;
	// printk("nfreepages after create_proc(): %d\n", nfreepages);
}

void
run_proc(struct Proc *proc)
{
	// if (proc != procs)
	//     printk("run_proc: %d\n", proc - procs);
	curproc = proc;
	struct ProcContext *context = &proc->context;
	lcr3((u64)proc->pgtbl);
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
	for (int i = 0; i < 64; ++i) {
		proc->fdesc[i].inuse = 0;
	}
	// printk("proc[%d] exec time: %d timer periods.\n", proc - procs, proc->exec_time);
	proc->exec_time = 0;
	if (proc == kbd_proc)
		kbd_proc = NULL;
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
	if (*(u32 *)ehdr->e_ident != ELF_MAGIC) {
		return -E_FORMAT;
	}
	struct Elf64_Phdr *phdr = (struct Elf64_Phdr *)(img + ehdr->e_phoff);
	for (int i = 0; i < ehdr->e_phnum; ++i, ++phdr) {
		if (phdr->p_type != PT_LOAD)
			continue;
		char *src = img + phdr->p_offset, 
			 *src_fileend = src + phdr->p_filesz,
			 *src_memend = src + phdr->p_memsz;
		u64 vaddr = phdr->p_vaddr;
		pte_t *pte = NULL;
		while (src < src_memend) {
			if (pte == NULL || (vaddr & (PGSIZE - 1)) == 0) {
				int ret = walk_pgtbl(proc->pgtbl, PAGEADDR(vaddr), &pte, 1);
				if (ret) {
					free_pgtbl(proc->pgtbl, FREE_PGTBL_DECREF);
					return -E_NOMEM;
				}
				*pte |= PTE_U | PTE_W;
			}
			if (src <= src_fileend) {
				*(char *)(PAGEKADDR(*pte) + (vaddr & (PGSIZE - 1))) = *src;
			}
			src++;
			vaddr++;
		}
	}
	return 0;
}