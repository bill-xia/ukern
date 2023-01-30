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
#include "string.h"
#include "errno.h"

// check inside current pagetable:
// whether `flags` are set for PTEs in memory range [addr,addr+siz]
// PTE_P is neccessary, add other flags on need. (PTE_U and PTE_W are common)
// Returns 1 on success, 0 on failure.
int
check_mem(u64 addr, u64 siz, u64 flags)
{
	flags |= PTE_P;
	addr = PAGEADDR(addr);
	u64 end = PAGEADDR(addr + siz) + PGSIZE;
	u64 *pte;
	pgtbl_t pgtbl = (pgtbl_t)P2K(rcr3());
	while (addr != end) {
		walk_pgtbl(pgtbl, addr, &pte, 0);
		if (pte == NULL || (*pte & flags) != flags) {
			return 0;
		}
		addr += PGSIZE;
	}
	return 1;
}

// check inside current pagetable:
// whether `flags` are set for PTEs in string beginning at `addr`. Check `siz`
// bytes at most.
// PTE_P is neccessary, add other flags on need. (PTE_U and PTE_W are common)
// Returns 1 on success, 0 on failure.
int
check_str(u64 addr, u64 siz, u64 flags)
{
	// printk("check_str addr: %lx, siz %d, flags %lx\n", addr, siz, flags);
	u64 str_offset = addr & PGMASK;

	flags |= PTE_P;
	addr = PAGEADDR(addr);
	u64 end = PAGEADDR(addr + siz) + PGSIZE;
	u64 *pte;
	pgtbl_t pgtbl = (pgtbl_t)P2K(rcr3());
	while (addr != end) {
		walk_pgtbl(pgtbl, addr, &pte, 0);
		if (pte == NULL || (*pte & flags) != flags) {
			return 0;
		}
		char *str = (char *)PAGEKADDR(*pte) + str_offset;
		for (int i = 0; i < PGSIZE - str_offset; ++i) {
			if (str[i] == 0)
				return 1;
		}
		str_offset = 0;
		addr += PGSIZE;
	}
	return 0;
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

	// add to living_child
	if (curproc->living_child == NULL) {
		curproc->living_child = nproc;
		nproc->prev_sibling = nproc->next_sibling = NULL;
	} else {
		struct Proc *ohead = curproc->living_child;
		ohead->prev_sibling = nproc;
		nproc->next_sibling = ohead;
		curproc->living_child = nproc;
	}
	nproc->parent = curproc;
	copy_pgtbl(nproc->pgtbl, curproc->p_pgtbl, CPY_PGTBL_CNTREF);
	// save the "real" page table
	copy_pgtbl(nproc->p_pgtbl, curproc->p_pgtbl, 0);
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
	if (!check_str(tf->rdx, 256, PTE_U)) { // TODO: max filename length
		tf->rax = -EFAULT;
		return;
	}
	u32 head_cluster, use_fat;
	u64 file_len;
	for (int fd = 0; fd < 256; ++fd) {
		if (curproc->fdesc[fd].inuse) continue;
		int r = open_file((void *)tf->rdx, curproc->fdesc + fd);
		if (r == 0) {
			tf->rax = fd;
			curproc->fdesc[fd].inuse = 1;
		} else {
			tf->rax = r;
		}
		return;
	}
	// no available file descriptor
	tf->rax = -EMFILE;
	return;
}

void
sys_read(struct ProcContext *tf)
{
	int fd = tf->rdx;
	if (fd < 0 || fd >= 64) {
		tf->rax = -EBADF;
		return;
	}
	if (!check_mem(tf->rcx, tf->rbx, PTE_U | PTE_W)) {
		tf->rax = -EFAULT;
		return;
	}
	struct file_desc *fdesc = &curproc->fdesc[fd];
	if (fdesc->inuse == 0) {
		tf->rax = -EBADF;
		return;
	}
	int r = read_file(
		(void *)tf->rcx,
		tf->rbx,
		fdesc
	);
	tf->rax = r;
	curproc->fdesc[fd].read_ptr += r;
}

void
sys_exec(struct ProcContext *tf)
{
	static char argv_buf[16][256];
	// save argv
	char **ori_argv = (char **)tf->rbx;
	int argc = (int)tf->rcx;
	if (argc > NARGS) {
		tf->rax = -EINVAL;
		return;
	}
	for (int i = 0; i < argc; ++i) {
		if (!check_str((u64)ori_argv[i], ARGLEN, PTE_U)) {
			tf->rax = -EINVAL;
			return;
		}
		strncpy(argv_buf[i], ori_argv[i], ARGLEN);
	}

	// open file
	struct file_desc fdesc;
	int ret = 0;
	int r = open_file((void *)(tf->rdx), &fdesc);
	if (r) { // open file failed
		printk("exec: open file failed.\n");
		tf->rax = r;
		return;
	}

	// read image
	char *img = (char *)EXEC_IMG;
	u64 addr = EXEC_IMG, addr_end = EXEC_IMG + fdesc.file_len;
	lcr3(K2P(k_pgtbl));
	// if (img < img_end) {
		// file too large, seldom happen
	// }
	while (addr < addr_end) {
		pte_t *pte;
		if (ret = walk_pgtbl(k_pgtbl, addr, &pte, 1)) {
			tf->rax = -ENOMEM;
			addr_end = addr;
			goto free_img;
		}
		*pte |= PTE_U | PTE_W;
		addr += PGSIZE;
	}
	lcr3(rcr3());
	read_file(img, fdesc.file_len, &fdesc);
	// TODO: check file format before destroying current enviroment,
	// return enough information if format error

	curproc->state = PENDING;
	// from now on, old process image is destroyed
	free_pgtbl(curproc->pgtbl, FREE_PGTBL_DECREF);
	free_pgtbl(curproc->p_pgtbl, 0);
	struct page_info *page = alloc_page(FLAG_ZERO);
	curproc->pgtbl = (pgtbl_t)page->paddr;
	page = alloc_page(FLAG_ZERO);
	curproc->p_pgtbl = (pgtbl_t)page->paddr;
	// code below is copied from create_proc()
	// may delete create_proc() in the future
	struct Proc *proc = curproc;
	if (ret = load_img(img, proc)) {
		kill_proc(proc);
		goto free_img;
	}
	// stack
	pte_t *pte;
	if (ret = walk_pgtbl(proc->pgtbl, USTACK - PGSIZE, &pte, 1)) {
		kill_proc(proc);
		goto free_img;
	}
	*pte |= PTE_U | PTE_W;
	// uargs
	char **uargv = (char **)(PAGEKADDR(*pte) + PGSIZE - NARGS*sizeof(u64));
	for (int i = 0; i < NARGS; ++i) {
		uargv[i] = (char *)((u64)UARGS + 256 * i);
	}
	if (ret = walk_pgtbl(proc->pgtbl, UARGS, &pte, 1)) {
		kill_proc(proc);
		goto free_img;
	}
	*pte |= PTE_U | PTE_W;
	char *uarg_pg = (char *)PAGEKADDR(*pte);
	for (int i = 0; i < NARGS; ++i) {
		memcpy(uarg_pg + i * ARGLEN, argv_buf[i], ARGLEN);
	}
	// mapping kernel space
	pgtbl_t pgtbl = (pgtbl_t)P2K(proc->pgtbl);
	for (int i = 256; i < 512; ++i) {
		pgtbl[i] = k_pgtbl[i];
	}
	copy_pgtbl(proc->p_pgtbl, proc->pgtbl, 0);
	// set up runtime enviroment
	struct Elf64_Ehdr *ehdr = (struct Elf64_Ehdr *)img;
	clear_proc_context(&proc->context);
	proc->context.rip = ehdr->e_entry;
	proc->context.cs = USER_CODE_SEL | 3;
	proc->context.rsp = USTACK - NARGS * sizeof(u64);
	proc->context.ss = USER_DATA_SEL | 3;
	proc->context.rflags = 0x02 | RFLAGS_IF | (3 << RFLAGS_IOPL_SHIFT);
	// argc in rdi
	proc->context.rdi = tf->rcx;
	// argv in rsi, which is predefined
	proc->context.rsi = USTACK - NARGS * sizeof(u64);
	proc->state = READY;

	// free pages for img
free_img:
	addr = EXEC_IMG;
	while (addr < addr_end) {
		pte_t *pte;
		ret = walk_pgtbl(k_pgtbl, addr, &pte, 0);
		assert(pte != NULL && ret == 0);
		free_page(PA2PGINFO(*pte));
		*pte = 0;
		addr += PGSIZE;
	}

	sched();
}

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
		tf->rax = -EBUSY; // keyboard in use
		return;
	}
	curproc->context = *tf;
	curproc->state = PENDING;
	sched();
}

void
exit_as_parent()
{
	struct Proc	*living_child = curproc->living_child,
			*zombie_child = curproc->zombie_child,
			*nxt;
	while (living_child != NULL) {
		living_child->parent = NULL;
		living_child = living_child->next_sibling;
	}
	while (zombie_child != NULL) {
		nxt = zombie_child->next_sibling;
		kill_proc(zombie_child);
		zombie_child = nxt;
	}
}

void
exit_as_child()
{
	struct Proc *parent = curproc->parent;
	if (parent == NULL) {
		// parent is dead
		kill_proc(curproc);
		return;
	}
	if (!parent->waiting) {
		// zombie
		// add to zombie_child
		if (parent->zombie_child == NULL) {
			parent->zombie_child = curproc;
			curproc->prev_sibling = curproc->next_sibling = NULL;
		} else {
			struct Proc *ohead = parent->zombie_child;
			ohead->prev_sibling = curproc;
			curproc->next_sibling = ohead;
			parent->zombie_child = curproc;
		}

		curproc->state = ZOMBIE;
		return;
	}
	// wait()ed by parent
	parent->waiting = 0;
	if (parent->wait_status != NULL) {
		*parent->wait_status = 0; // TODO: copy on write?
	}
	// invoke parent
	parent->context.rax = curproc->pid;
	parent->state = READY;
	// detach from the sibling chain
	if (curproc->prev_sibling) {
		curproc->prev_sibling->next_sibling = curproc->next_sibling;
	}
	if (curproc->next_sibling) {
		curproc->next_sibling->prev_sibling = curproc->prev_sibling;
	}
	// hand over sibling chain head
	if (parent->living_child == curproc) {
		parent->living_child = curproc->next_sibling;
	}
	if (parent->zombie_child == curproc) {
		parent->zombie_child = curproc->next_sibling;
	}
	kill_proc(curproc);
}

void
sys_exit()
{
	exit_as_parent();
	exit_as_child();
	sched();
}

void
sys_wait(struct ProcContext *tf)
{
	if (tf->rdx != 0 && !check_mem(tf->rdx, sizeof(int), PTE_U | PTE_W)) {
		tf->rax = -EFAULT;
		return;
	}
	if (curproc->zombie_child != NULL) {
		// have zombie child, wait() it
		if (tf->rdx != 0)
			*(int *)(tf->rdx) = 0; // TODO: more wait_status
		struct Proc	*ohead = curproc->zombie_child,
				*nhead = curproc->zombie_child->next_sibling;
		// return child's pid
		tf->rax = ohead->pid;
		curproc->zombie_child = nhead;
		if (nhead)
			nhead->prev_sibling = NULL;
		kill_proc(ohead);
		return;
	}
	if (curproc->living_child == NULL) {
		// no child at all
		tf->rax = -ECHILD;
		return;
	}
	// sleep, wait for living child
	curproc->waiting = 1;
	curproc->wait_status = (int *)tf->rdx;
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
		sys_exit();
		break;
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
	case 9:
		sys_wait(tf);
		break;
	default:
		printk("unknown syscall\n");
		while (1);
	}
}