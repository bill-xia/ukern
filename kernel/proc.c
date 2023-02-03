#include "proc.h"
#include "mem.h"
#include "elf64.h"
#include "x86.h"
#include "printk.h"
#include "sched.h"
#include "errno.h"
#include "sse.h"

struct fx_context *fx_ctxs;
struct proc *procs, *curproc, *kbd_proc;
static u64 g_pid;

void
clear_proc_context(struct proc_context *context)
{
	memset(context, 0, sizeof(struct proc_context));
}

void
init_pcb(void)
{
	procs = (struct proc *)ROUNDUP((u64)end_kmem, sizeof(struct proc));
	memset(procs, 0, NPROCS * sizeof(struct proc));
	end_kmem = (char *)(procs + NPROCS);
	fx_ctxs = (struct fx_context *)ROUNDUP((u64)end_kmem, PGSIZE);
	memset(fx_ctxs, 0, NPROCS * sizeof(struct fx_context));
	for (int i = 0; i < NPROCS; ++i) {
		fx_ctxs[i].MXCSR = MXCSR_MASKALL;
	}
	end_kmem = (char *)(fx_ctxs + NPROCS);
}

struct proc *
alloc_proc(void)
{
	struct proc *proc;
	for (proc = procs; proc < procs + NPROCS; ++proc) {
		if (proc->state != CLOSE) {
			continue;
		}
		// TODO: add lock here when multi-core is up
		struct page_info *page = alloc_page(FLAG_ZERO);
		proc->pgtbl = (pgtbl_t)page->paddr;
		page = alloc_page(FLAG_ZERO);
		proc->p_pgtbl = (pgtbl_t)page->paddr;
		proc->state = PENDING;
		proc->pid = ++g_pid;
		return proc;
	}
	return NULL;
}

// Create a process according to executable image `img`. Only executed on boot.
// Returns:
//	0 on success
//	-EAGAIN if limit on the number of processes is encountered
int
create_proc(char *img)
{
	int r = 0;
	// printk("nfreepages before create_proc(): %d\n", nfreepages);
	struct proc *proc = alloc_proc();
	if (proc == NULL) {
		return -EAGAIN;
	}
	assert((r = load_img(img, proc)) == 0);
	// stack
	pte_t *pte;
	if ((r = walk_pgtbl(proc->pgtbl, USTACK - PGSIZE, &pte, 1)) < 0) {
		panic("create_proc() failed: %e\n", r);
	}
	assert(*pte == 0);
	struct page_info *page = alloc_page(FLAG_ZERO);
	*pte = page->paddr | PTE_P | PTE_U | PTE_W;
	// uargs
	char **uargv = (char **)(PAGEKADDR(*pte) + PGSIZE - NARGS * sizeof(u64));
	for (int i = 0; i < NARGS; ++i) {
		uargv[i] = (char *)((u64)UARGS + 256 * i);
	}
	assert((r = walk_pgtbl(proc->pgtbl, UARGS, &pte, 1)) == 0);
	assert(*pte == 0);
	page = alloc_page(FLAG_ZERO);
	*pte = page->paddr | PTE_P | PTE_U | PTE_W;
	// mapping kernel space
	for (int i = 256; i < 512; ++i) {
		proc->pgtbl[i] = k_pgtbl[i];
	}
	copy_pgtbl(proc->p_pgtbl, proc->pgtbl, 0);
	// set up pwd
	open_dir("/", NULL, &proc->pwd);
	proc->fdesc[0].file_type = FT_KBD;
	proc->fdesc[0].inuse = 1;
	proc->fdesc[1].file_type = FT_SCREEN;
	proc->fdesc[1].inuse = 1;
	proc->fdesc[2].file_type = FT_SCREEN;
	proc->fdesc[2].inuse = 1;
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
	return 0;
	// printk("nfreepages after create_proc(): %d\n", nfreepages);
}

void
run_proc(struct proc *proc)
{
	// if (proc != procs)
	//     printk("run_proc: %d\n", proc - procs);
	curproc = proc;
	struct proc_context *context = &proc->context;
	struct fx_context *fx_ctx = fx_ctxs + (proc - procs);
	lcr3((u64)proc->pgtbl);
	asm volatile (
		"fxrstorq (%0)\n"
		"mov %1, %%rsp\n"
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
		:: "r"(fx_ctx), "m"(context) : "memory");
}

void
exit_as_parent(struct proc *proc)
{
	struct proc	*living_child = proc->living_child,
			*zombie_child = proc->zombie_child,
			*nxt;
	while (living_child != NULL) {
		living_child->parent = NULL;
		living_child = living_child->next_sibling;
	}
	while (zombie_child != NULL) {
		nxt = zombie_child->next_sibling;
		clear_proc(zombie_child);
		zombie_child = nxt;
	}
}

void
exit_as_child(struct proc *proc, i64 exit_val)
{
	struct proc *parent = proc->parent;
	if (parent == NULL) {
		// parent is dead
		clear_proc(proc);
		return;
	}
	if (!parent->waiting) {
		// zombie
		// add to zombie_child
		if (parent->zombie_child == NULL) {
			parent->zombie_child = proc;
			proc->prev_sibling = proc->next_sibling = NULL;
		} else {
			struct proc *ohead = parent->zombie_child;
			ohead->prev_sibling = proc;
			proc->next_sibling = ohead;
			parent->zombie_child = proc;
		}
		proc->exit_val = exit_val;
		proc->state = ZOMBIE;
		return;
	}
	// wait()ed by parent
	parent->waiting = 0;
	if (parent->wait_status != NULL) {
		i64 *wstatus = (i64 *)U2K(parent, (u64)parent->wait_status, 1);
		*wstatus = exit_val;
	}
	// invoke parent
	parent->context.rax = proc->pid;
	parent->state = READY;
	// detach from the sibling chain
	if (proc->prev_sibling) {
		proc->prev_sibling->next_sibling = proc->next_sibling;
	}
	if (proc->next_sibling) {
		proc->next_sibling->prev_sibling = proc->prev_sibling;
	}
	// hand over sibling chain head
	if (parent->living_child == proc) {
		parent->living_child = proc->next_sibling;
	}
	if (parent->zombie_child == proc) {
		parent->zombie_child = proc->next_sibling;
	}
	clear_proc(proc);
}

// Don't use this function if you're not sure what it does: maybe you just want
// a `memset(proc, 0, sizeof(struct proc));`
//
// Clean up a process, no one cares about it any more.
// Note that zombie won't be cleared, see kill_proc() and exit_as_child().
void
clear_proc(struct proc *proc)
{
	// Don't allocate this struct proc yet, thus don't mark as CLOSE.
	// Meaningless in a single-processor system, but it's a must-have in
	// MP system.
	proc->state = PENDING;
	
	free_pgtbl((pgtbl_t)P2K(proc->pgtbl), FREE_PGTBL_DECREF);
	free_pgtbl((pgtbl_t)P2K(proc->p_pgtbl), 0);

	if (proc == kbd_proc)
		kbd_proc = NULL;

	for (int fd = 0; fd < 64; ++fd) {
		struct file_desc *fdesc = &proc->fdesc[fd];
		if (!fdesc->inuse)
			continue;
		struct pipe *pipe;
		switch (fdesc->file_type) {
		case FT_RPIPE: ;
			pipe = fdesc->meta_pipe.pipe;
			pipe->rref--;
			if (pipe->rref == 0 && pipe->wref == 0) {
				free_page(KA2PGINFO(pipe));
			}
			break;
		case FT_WPIPE:
			pipe = fdesc->meta_pipe.pipe;
			pipe->wref--;
			if (pipe->wref == 0 && pipe->reader != NULL) {
				// wake up reader, nothing will be written any more
				pipe->reader->context.rax = 0;
				pipe->reader->state = READY;
			}
			if (pipe->rref == 0 && pipe->wref == 0) {
				free_page(KA2PGINFO(pipe));
			}
			break;
		default:
			break;
		}
	}
	proc->exit_val = 0;
	// fish back to the sea
	memset(proc, 0, sizeof(struct proc));
	memset(fx_ctxs + (proc - procs), 0, sizeof(struct fx_context));
	fx_ctxs[proc - procs].MXCSR = MXCSR_MASKALL;
}

// Kill the process, it may become a zombie! (sounds scary...)
void
kill_proc(struct proc *proc, i64 exit_val)
{
	proc->exit_val = exit_val;
	exit_as_parent(proc);
	exit_as_child(proc, exit_val);
}

int
check_img_format(char *img)
{
	struct Elf64_Ehdr *ehdr = (struct Elf64_Ehdr *)img;
	if (*(u32 *)ehdr->e_ident != ELF_MAGIC) {
		return -ENOEXEC;
	}
	return 0;
}

// Load program image pointed by `ehdr`
// into `proc`'s memory space.
// Returns:
//	0 on success
//	-ENOMEM when memory not enough
//	-ENOEXEC when img is not ELF64 file
int
load_img(char *img, struct proc *proc)
{
	struct Elf64_Ehdr *ehdr = (struct Elf64_Ehdr *)img;
	if (*(u32 *)ehdr->e_ident != ELF_MAGIC) {
		return -ENOEXEC;
	}
	struct Elf64_Phdr *phdr = (struct Elf64_Phdr *)(img + ehdr->e_phoff);
	for (int i = 0; i < ehdr->e_phnum; ++i, ++phdr) {
		if (phdr->p_type != PT_LOAD)
			continue;
		char	*src = img + phdr->p_offset, 
			*src_fileend = src + phdr->p_filesz,
			*src_memend = src + phdr->p_memsz;
		u64 vaddr = phdr->p_vaddr;
		pte_t *pte = NULL;
		while (src < src_memend) {
			if (pte == NULL || (vaddr & (PGSIZE - 1)) == 0) {
				int ret = walk_pgtbl(proc->pgtbl, PAGEADDR(vaddr), &pte, 1);
				if (ret) {
					free_pgtbl(proc->pgtbl, FREE_PGTBL_DECREF);
					return -ENOMEM;
				}
				struct page_info *page = alloc_page(FLAG_ZERO);
				*pte = page->paddr | PTE_P | PTE_U | PTE_W;
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

u64
U2K(struct proc *proc, u64 vaddr, int write)
{
	pte_t *pte1, *pte2;
	walk_pgtbl(proc->pgtbl, vaddr, &pte1, 0);
	walk_pgtbl(proc->p_pgtbl, vaddr, &pte2, 0);
	assert((*pte1 & (PTE_P | PTE_U)) == (PTE_P | PTE_U));
	assert(PAGEADDR(*pte1) == PAGEADDR(*pte2));
	if (!write || (*pte1 & PTE_FLAGS) == (*pte2 & PTE_FLAGS)) {
		return PAGEKADDR(*pte1) | (vaddr & PGMASK);
	}
	// if there's any difference, it should be the write flag
	// dirty flag: haha
	assert(!(*pte1 & PTE_W) && (*pte2 & PTE_W));
	PA2PGINFO(*pte2)->ref--;
	struct page_info *page = alloc_page(0);
	char	*src = (char *)PAGEKADDR(*pte2),
		*dst = (char *)PAGEKADDR(page->paddr);
	for (int i = 0; i < PGSIZE; ++i)
		dst[i] = src[i];
	*pte1 = page->paddr | PTE_P | PTE_U | PTE_W;
	*pte2 = page->paddr | PTE_P | PTE_U | PTE_W;
	return PAGEKADDR(*pte1) | (vaddr & PGMASK);
}

void
fx_save(struct proc *proc)
{
	struct fx_context *fx_ctx = fx_ctxs + (proc - procs);
	asm volatile (
		"fxsaveq (%0)"
		: : "r"(fx_ctx)
	);
	return;
}

void
fx_restore(struct proc *proc)
{
	struct fx_context *fx_ctx = fx_ctxs + (proc - procs);
	asm volatile (
		"fxrstorq (%0)"
		: : "r"(fx_ctx)
	);
	return;
}