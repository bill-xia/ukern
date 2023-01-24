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
	PENDING
};

struct ProcContext {
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

struct Proc {
	pgtbl_t 		pgtbl,
				p_pgtbl;// p_pgtbl's corresponding mapping should be
					// different with pgtbl only at W flag, and
					// kernel space in p_pgtbl may be not mapped,
					// because we don't use them in p_pgtbl
	u64			pid;
	enum proc_state		state;
	u64			exec_time;
	struct ProcContext	context;
	struct file_desc	fdesc[64];
};
extern struct Proc *procs, *curproc, *kbd_proc;

void init_pcb(void);
int create_proc(char *img);
void run_proc(struct Proc *proc);
void kill_proc(struct Proc *proc);
struct Proc *alloc_proc(void);
int load_img(char *img, struct Proc *proc);

#define IMAGE_SYMBOL(x) _binary_obj_user_ ## x ## _start
#define CREATE_PROC(x) do { \
	extern char IMAGE_SYMBOL(x); \
	int r; \
	if ((r = create_proc(&IMAGE_SYMBOL(x))) < 0) { \
		panic("create_proc failed: %d\n", r); \
	} \
} while (0)

#endif