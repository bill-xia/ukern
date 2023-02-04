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
#include "dirent.h"
#include "ukern.h"
#include "sse.h"

// check inside pgtbl:
// whether `flags` are set for PTEs in memory range [addr,addr+siz]
// PTE_P is neccessary, add other flags on need. (PTE_U and PTE_W are common)
// Returns 1 on success, 0 on failure.
int
check_mem(pgtbl_t pgtbl, u64 addr, u64 siz, u64 flags)
{
	flags |= PTE_P;
	addr = PAGEADDR(addr);
	u64 end = PAGEADDR(addr + siz) + PGSIZE;
	u64 *pte;
	while (addr != end) {
		walk_pgtbl(pgtbl, addr, &pte, 0);
		if (pte == NULL || (*pte & flags) != flags) {
			return 0;
		}
		addr += PGSIZE;
	}
	return 1;
}

// check inside pgtbl:
// whether `flags` are set for PTEs in string beginning at `addr`. Check `siz`
// bytes at most.
// PTE_P is neccessary, add other flags on need. (PTE_U and PTE_W are common)
// Returns 1 on success, 0 on failure.
int
check_str(pgtbl_t pgtbl, u64 addr, u64 siz, u64 flags)
{
	// printk("check_str addr: %lx, siz %d, flags %lx\n", addr, siz, flags);
	u64 str_offset = addr & PGMASK;

	flags |= PTE_P;
	addr = PAGEADDR(addr);
	u64 end = PAGEADDR(addr + siz) + PGSIZE;
	u64 *pte;
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
sys_fork(struct proc_context *tf)
{
	struct proc *nproc = alloc_proc();

	// add to living_child
	if (curproc->living_child == NULL) {
		curproc->living_child = nproc;
		nproc->prev_sibling = nproc->next_sibling = NULL;
	} else {
		struct proc *ohead = curproc->living_child;
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
	// copy fdesc
	for (int i = 0; i < 64; ++i) {
		nproc->fdesc[i] = curproc->fdesc[i];
		if (!curproc->fdesc[i].inuse)
			continue;
		if (curproc->fdesc[i].file_type == FT_RPIPE) {
			curproc->fdesc[i].meta_pipe.pipe->rref++;
		}
		if (curproc->fdesc[i].file_type == FT_WPIPE) {
			curproc->fdesc[i].meta_pipe.pipe->wref++;
		}
	}
	nproc->pwd = curproc->pwd;
	nproc->exec_time = curproc->exec_time;
	nproc->end_umem = curproc->end_umem;
	// copy context
	nproc->context = *tf;
	memcpy(fx_ctxs + (nproc - procs), fx_ctxs + (curproc - procs),
		sizeof(struct fx_context));
	// set return value
	tf->rax = nproc->pid;
	nproc->context.rax = 0;
	// set state
	nproc->state = READY;
	lcr3(rcr3());
}

void
sys_open(struct proc_context *tf)
{
	if (!check_str(curproc->p_pgtbl, tf->rdx, 256, PTE_U)) { // TODO: max filename length
		tf->rax = -EFAULT;
		return;
	}
	for (int fd = 0; fd < 256; ++fd) {
		if (curproc->fdesc[fd].inuse) continue;
		char *fn = (char *)tf->rdx;
		int r = open_file(fn, &curproc->pwd, curproc->fdesc + fd);
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
sys_read(struct proc_context *tf)
{
	int fd = tf->rdx;
	if (fd < 0 || fd >= 64) {
		tf->rax = -EBADF;
		return;
	}
	if (tf->rbx == 0) { // try to read 0 byte
		tf->rax = -EINVAL;
		return;
	}
	if (!check_mem(curproc->p_pgtbl, tf->rcx, tf->rbx, PTE_U | PTE_W)) {
		tf->rax = -EFAULT;
		return;
	}
	struct file_desc *fdesc = &curproc->fdesc[fd];
	if (fdesc->inuse == 0) {
		tf->rax = -EBADF;
		return;
	}
	int r;
	switch (fdesc->file_type) {
	case FT_KBD:
		if (kbd_buf_siz > 0) {
			// copy from kbd_buffer into user space
			u8 *dst = (u8 *)tf->rcx;
			r = min(kbd_buf_siz, tf->rbx);
			for (int i = 0; i < r; ++i) {
				dst[i] = (u8)kbd_buffer[kbd_buf_beg++];
				if (kbd_buf_beg == 4096)
					kbd_buf_beg = 0;
				kbd_buf_siz--;
			}
			// flush write-combined memory for framebuffer
			// doesn't seem useful yet, leave here in case
			// we need the flush
			// wbinvd(); 
			tf->rax = r;
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
		break;
	case FT_REG:
		r = read_file(
			(void *)tf->rcx,
			tf->rbx,
			fdesc
		);
		tf->rax = r;
		curproc->fdesc[fd].read_ptr += r;
		break;
	case FT_RPIPE:
		struct pipe *pipe = fdesc->meta_pipe.pipe;
		if (pipe->reader != NULL) {
			tf->rax = -EBUSY;
			return;
		}
		// mark we're listening
		pipe->reader = curproc;
		pipe->rsiz = tf->rbx;
		// just save the user space pointer, convert when we really need
		// it, or copy-on-write can update the pgtbl and the ptr can
		// point into another process
		pipe->rbuf = (void *)tf->rcx;
		if (pipe->len > 0) {
			// tf->rbx > 0 is guaranteed, do the read
			tf->rax = pipe_read(pipe, 0);
			return;
		}
		if (pipe->wref == 0) {
			// no writer, return 0
			tf->rax = 0;
			return;
		}
		curproc->context = *tf;
		curproc->state = PENDING;
		sched();
		break;
	default:
		tf->rax = -EBADF;
		break;
	}
}

void
sys_exec(struct proc_context *tf)
{
	static char argv_buf[16][256];
	struct page_info *page;
	int r;
	// save argv
	char **ori_argv = (char **)tf->rbx;
	int argc = (int)tf->rcx;
	if (argc > NARGS) {
		tf->rax = -EINVAL;
		return;
	}
	for (int i = 0; i < argc; ++i) {
		if (!check_str(curproc->p_pgtbl, (u64)ori_argv[i], ARGLEN, PTE_U)) {
			tf->rax = -EINVAL;
			return;
		}
		strncpy(argv_buf[i], ori_argv[i], ARGLEN);
	}

	// open file
	struct file_desc fdesc;
	if ((r = open_file((void *)(tf->rdx), &curproc->pwd, &fdesc)) < 0) { // open file failed
		tf->rax = r;
		return;
	}

	// read image
	char *img = (char *)EXEC_IMG;
	u64 addr = EXEC_IMG, addr_end = EXEC_IMG + fdesc.file_len;
	// if (img < img_end) {
		// file too large, seldom happen
	// }
	while (addr < addr_end) {
		pte_t *pte;
		if ((r = walk_pgtbl(k_pgtbl, addr, &pte, 1)) < 0) {
			tf->rax = -ENOMEM;
			addr_end = addr;
			goto free_img;
		}
		assert(*pte == 0);
		page = alloc_page(FLAG_ZERO);
		*pte = page->paddr | PTE_P;
		addr += PGSIZE;
	}
	// lcr3(rcr3()); // TODO: do we need to flush this?
	read_file(img, fdesc.file_len, &fdesc);
	// check file format before destroying current enviroment,
	// return enough information if format error
	if ((r = check_img_format(img)) < 0) {
		tf->rax = -ENOEXEC;
		addr = EXEC_IMG;
		while (addr < addr_end) {
			pte_t *pte;
			r = walk_pgtbl(k_pgtbl, addr, &pte, 0);
			assert(pte != NULL && r == 0);
			free_page(PA2PGINFO(*pte));
			*pte = 0;
			addr += PGSIZE;
		}
		return;
	}

	curproc->state = PENDING;
	// from now on, old process image is destroyed
	lcr3(K2P(k_pgtbl));
	free_pgtbl(curproc->pgtbl, FREE_PGTBL_DECREF);
	free_pgtbl(curproc->p_pgtbl, 0);

	page = alloc_page(FLAG_ZERO);
	curproc->pgtbl = (pgtbl_t)page->paddr;
	page = alloc_page(FLAG_ZERO);
	curproc->p_pgtbl = (pgtbl_t)page->paddr;
	// code below is copied from create_proc()
	// may delete create_proc() in the future
	struct proc *proc = curproc;
	if ((r = load_img(img, proc)) < 0) {
		kill_proc(proc, r);
		goto free_img;
	}
	// stack
	pte_t *pte;
	if ((r = walk_pgtbl(proc->pgtbl, USTACK - PGSIZE, &pte, 1)) < 0) {
		kill_proc(proc, r);
		goto free_img;
	}
	assert(*pte == 0);
	page = alloc_page(FLAG_ZERO);
	*pte = page->paddr | PTE_P | PTE_U | PTE_W;
	// uargs
	char **uargv = (char **)(PAGEKADDR(*pte) + PGSIZE - NARGS*sizeof(u64));
	for (int i = 0; i < NARGS; ++i) {
		uargv[i] = (char *)((u64)UARGS + 256 * i);
	}
	if ((r = walk_pgtbl(proc->pgtbl, UARGS, &pte, 1)) < 0) {
		kill_proc(proc, r);
		goto free_img;
	}
	assert(*pte == 0);
	page = alloc_page(FLAG_ZERO);
	*pte = page->paddr | PTE_P | PTE_U | PTE_W;
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
		r = walk_pgtbl(k_pgtbl, addr, &pte, 0);
		assert(pte != NULL && r == 0);
		free_page(PA2PGINFO(*pte));
		*pte = 0;
		addr += PGSIZE;
	}

	memset(fx_ctxs + (proc - procs), 0, sizeof(struct fx_context));
	fx_ctxs[proc - procs].MXCSR = MXCSR_MASKALL;
	sched();
}

void
sys_exit(i64 exit_val)
{
	kill_proc(curproc, exit_val);
	sched();
}

void
sys_wait(struct proc_context *tf)
{
	if (tf->rdx != 0 && !check_mem(curproc->p_pgtbl, tf->rdx, sizeof(i64), PTE_U | PTE_W)) {
		tf->rax = -EFAULT;
		return;
	}
	if (curproc->zombie_child != NULL) {
		// have zombie child, wait() it
		struct proc	*ohead = curproc->zombie_child,
				*nhead = curproc->zombie_child->next_sibling;
		if (tf->rdx != 0)
			*(i64 *)(tf->rdx) = ohead->exit_val; // TODO: more wait_status
		// return child's pid
		tf->rax = ohead->pid;
		curproc->zombie_child = nhead;
		if (nhead)
			nhead->prev_sibling = NULL;
		clear_proc(ohead);
		return;
	}
	if (curproc->living_child == NULL) {
		// no child at all
		tf->rax = -ECHILD;
		return;
	}
	// sleep, wait for living child
	curproc->waiting = 1;
	// just save the user space pointer, convert when we really need it,
	// or copy-on-write can update the pgtbl and the ptr can point into
	// another process
	curproc->wait_status = (i64 *)tf->rdx;

	curproc->context = *tf;
	curproc->state = PENDING;
	sched();
}

void
sys_opendir(struct proc_context *tf)
{
	if (!check_str(curproc->p_pgtbl, tf->rdx, 256, PTE_U)) { // TODO: max filename length
		tf->rax = -EFAULT;
		return;
	}
	for (int fd = 0; fd < 256; ++fd) {
		if (curproc->fdesc[fd].inuse) continue;
		int r = open_dir((void *)tf->rdx, &curproc->pwd, curproc->fdesc + fd);
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
sys_readdir(struct proc_context *tf)
{
	int fd = tf->rdx;
	if (fd < 0 || fd >= 64) {
		tf->rax = -EBADF;
		return;
	}
	if (!check_mem(curproc->p_pgtbl, tf->rcx, sizeof(struct dirent), PTE_U | PTE_W)) {
		tf->rax = -EFAULT;
		return;
	}
	struct file_desc *fdesc = &curproc->fdesc[fd];
	if (fdesc->inuse == 0 || fdesc->file_type != FT_DIR) {
		tf->rax = -EBADF;
		return;
	}
	int r = read_dir(
		(void *)tf->rcx,
		fdesc
	);
	tf->rax = r;
	return;
}

void
sys_chdir(struct proc_context *tf)
{
	if (!check_str(curproc->p_pgtbl, tf->rdx, 256, PTE_U)) { // TODO: max filename length
		tf->rax = -EFAULT;
		return;
	}
	struct file_desc newpwd;
	int r = open_dir((void *)tf->rdx, &curproc->pwd, &newpwd);
	if (r < 0) {
		tf->rax = r;
	} else {
		curproc->pwd = newpwd;
		tf->rax = 0;
	}
	return;
}

void
sys_write(struct proc_context *tf)
{
	int fd = tf->rdx;
	if (fd < 0 || fd >= 64) {
		tf->rax = -EBADF;
		return;
	}
	if (tf->rbx == 0) {
		tf->rbx = -EINVAL;
		return;
	}
	if (!check_mem(curproc->p_pgtbl, tf->rcx, tf->rbx, PTE_U)) {
		tf->rax = -EFAULT;
		return;
	}
	struct file_desc *fdesc = &curproc->fdesc[fd];
	if (fdesc->inuse == 0) {
		tf->rax = -EBADF;
		return;
	}
	size_t n;
	switch (fdesc->file_type) {
	case FT_SCREEN:
		char *buf = (char *)tf->rcx;
		n = tf->rbx;
		for (size_t i = 0; i < n; ++i) {
			console_putch(buf[i]);
		}
		tf->rax = n;
		break;
	// case FT_REG:
	case FT_WPIPE: ;
		struct pipe *pipe = fdesc->meta_pipe.pipe;
		if (pipe->writer != NULL) {
			tf->rax = -EBUSY;
			return;
		}
		// mark we're writing
		pipe->writer = curproc;
		pipe->wsiz = tf->rbx;
		pipe->wptr = 0;
		// just save the user space pointer, convert when we really need
		// it,  or copy-on-write can update the pgtbl and the ptr can
		// point into another process
		pipe->wbuf = (void *)tf->rcx;
		// do the write
		n = pipe_write(pipe, 0);
		assert(n <= tf->rbx);
		if (n == tf->rbx) { // success immediately
			tf->rax = n;
			return;
		}
		// stuck, wait for somebody to read out the pipe
		curproc->context = *tf;
		curproc->state = PENDING;
		sched();
		break;
	default:
		tf->rax = -EBADF;
		break;
	}
}

void
sys_pipe(struct proc_context *tf)
{
	if (!check_str(curproc->p_pgtbl, tf->rdx, sizeof(int[2]), PTE_U)) {
		tf->rax = -EFAULT;
		return;
	}
	// alloc the pipe
	struct page_info *pg = alloc_page(FLAG_ZERO);
	struct pipe *pipe = (struct pipe *)P2K(pg->paddr);
	pipe->rref = 1;
	pipe->wref = 1;

	int *fds = (int *)tf->rdx;
	int nfd = 0;
	for (int fd = 0; fd < 256; ++fd) {
		if (curproc->fdesc[fd].inuse) continue;
		fds[nfd] = fd;
		curproc->fdesc[fd].inuse = 1;
		curproc->fdesc[fd].file_type = (nfd == 0 ? FT_RPIPE : FT_WPIPE);
		curproc->fdesc[fd].meta_pipe.pipe = pipe;
		nfd++;
		if(nfd == 2) {
			tf->rax = 0;
			return;
		}
	}
	// release the pipe
	free_page(pg);
	// and the fdesc(s)
	for (int fd = 0; fd < nfd; ++fd) {
		curproc->fdesc[fd].inuse = 0;
	}
	return;
}

void
sys_close(struct proc_context *tf)
{
	int fd = tf->rdx;
	if (fd < 0 || fd >= 64 || !curproc->fdesc[fd].inuse) {
		tf->rax = -EBADF;
		return;
	}
	struct file_desc *fdesc = &curproc->fdesc[fd];
	struct pipe *pipe;
	switch (fdesc->file_type) {
	case FT_RPIPE:
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
	fdesc[fd].inuse = 0;
	return;
}

void
sys_sbrk(struct proc_context *tf)
{
	u64	inc = tf->rdx; // increment `inc` bytes
	u64	o_pgend = ROUNDUP(curproc->end_umem, PGSIZE),
		n_pgend = ROUNDUP(curproc->end_umem + inc, PGSIZE),
		new_pages = (n_pgend - o_pgend) / PGSIZE - nfreepages;
	if (n_pgend >= UMAX
		|| (i64)new_pages >= (i64)(nfreepages - FREEPAGES_LOWWATER)) {
		tf->rax = 0;
		// TODO: set errno to -ENOMEM
		return;
	}
	for (u64 vaddr = o_pgend; vaddr < n_pgend; vaddr += PGSIZE) {
		struct page_info *pg = alloc_page(FLAG_ZERO);
		pte_t *pte;
		walk_pgtbl(curproc->pgtbl, vaddr, &pte, 1);
		*pte = pg->paddr | PTE_U | PTE_P | PTE_W;
		walk_pgtbl(curproc->p_pgtbl, vaddr, &pte, 1);
		*pte = pg->paddr | PTE_U | PTE_P | PTE_W;
	}
	tf->rax = curproc->end_umem;
	curproc->end_umem += inc;
	return;
}

void
syscall(struct proc_context *tf)
{
	int num = tf->rax;
	switch (num) {
	case SYS_exit:
		sys_exit(tf->rdx);
		break;
	case SYS_fork:
		sys_fork(tf);
		break;
	case SYS_exec:
		sys_exec(tf);
		break;
	case SYS_wait:
		sys_wait(tf);
		break;
	case SYS_open:
		sys_open(tf);
		break;
	case SYS_read:
		sys_read(tf);
		break;
	case SYS_write:
		sys_write(tf);
		break;
	case SYS_close:
		sys_close(tf);
		break;
	case SYS_opendir:
		sys_opendir(tf);
		break;
	case SYS_readdir:
		sys_readdir(tf);
		break;
	case SYS_chdir:
		sys_chdir(tf);
		break;
	case SYS_pipe:
		sys_pipe(tf);
		break;
	case SYS_sbrk:
		sys_sbrk(tf);
		break;
	default:
		printk("unknown syscall %d\n", num);
		kill_proc(curproc, -0xFF);
		break;
	}
}