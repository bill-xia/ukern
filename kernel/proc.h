#ifndef PROC_H
#define PROC_H

#include "types.h"
#include "fs/fs.h"

#define NPROCS	1024
#define NARGS	16
#define ARGLEN	256

enum proc_state {
	CLOSE,
	READY,
	RUNNING,
	PENDING,
	ZOMBIE
};

struct proc_context {
	u64	rax,
		rcx,
		rdx,
		rbx;
	u64	rdi,
		rsi,
		rbp,
		zero;
	u64	r8,
		r9,
		r10,
		r11;
	u64	r12,
		r13,
		r14,
		r15;
	u64	vecnum,
		errno;
	u64	rip,
		cs;
	u64	rflags;
	u64	rsp,
		ss;
};

struct xmm_reg {
	u64	lo,
		hi;
} __attribute__ ((packed));

struct fx_context {
	u16	FCW,
		CSW;
	u8	FTW,
		rsv;
	u16	FOP;
	u32	FPU_IP1,
		FPU_IP2,
		FPU_DP1,
		FPU_DP2,
		MXCSR,
		MXCSR_MASK;
	struct xmm_reg	mm0,
			mm1,
			mm2,
			mm3,
			mm4,
			mm5,
			mm6,
			mm7,
			xmm0,
			xmm1,
			xmm2,
			xmm3,
			xmm4,
			xmm5,
			xmm6,
			xmm7,
			xmm8,
			xmm9,
			xmm10,
			xmm11,
			xmm12,
			xmm13,
			xmm14,
			xmm15;
} __attribute__ ((aligned(512)));
extern struct fx_context *fx_ctxs;

struct proc {
	struct proc_context	context;
	pgtbl_t 		pgtbl,
				p_pgtbl;// p_pgtbl's corresponding mapping should be
					// different with pgtbl only at W flag, and
					// kernel space in p_pgtbl may be not mapped,
					// because we don't use them in p_pgtbl
	u64			pid;
	enum proc_state		state;
	u64			exec_time;
	struct file_desc	pwd;
	i64			*wait_status;
	// About these pointers:
	// Relation between processes can be modeled as a tree,
	//	where parent and child refer to the process who
	//	called fork() and is fork()-ed, respectively.
	// `parent` points to its parent process, of course.
	// For one process, it has a pointer `living_children`
	//	which points to its *first* child that is alive,
	//	`zombie_children` is the counterpart points to
	//	its first zombie child.
	// `next_sibling` points to its next sibling, no matter
	//	this process is on its parent's living_children
	//	list or zombie_children list. It cannot sit on
	//	both of them, so it's fine. `prev_sibling` is similar.
	// `living_children` and `zombie_children` are kind of "owned"
	//	by the process itself, while `prev_sibling` and
	//	`next_sibling` are "owned" by the parent process.
	struct proc		*parent,
				*living_child,
				*zombie_child,
				*prev_sibling,
				*next_sibling;
	i64			exit_val;
	u64			waiting:1;
	struct file_desc	fdesc[64];
};
extern struct proc *procs, *curproc, *kbd_proc;

void init_pcb(void);
void clear_proc_context(struct proc_context *context);
int create_proc(char *img);
void run_proc(struct proc *proc);
void kill_proc(struct proc *proc, i64 exit_val);
void clear_proc(struct proc *proc);
struct proc *alloc_proc(void);
int load_img(char *img, struct proc *proc);
int check_img_format(char *img);
u64 U2K(struct proc *proc, u64 vaddr, int write);
void fx_save(struct proc *proc);
void fx_restore(struct proc *proc);

#define IMAGE_SYMBOL(x) _binary_obj_user_ ## x ## _start
#define CREATE_PROC(x) do { \
	extern char IMAGE_SYMBOL(x); \
	int r; \
	if ((r = create_proc(&IMAGE_SYMBOL(x))) < 0) { \
		panic("create_proc failed: %d\n", r); \
	} \
} while (0)

#endif