#ifndef PROC_H
#define PROC_H

#include "types.h"
#include "fs.h"

#define NPROCS 1024
#define NARGS  16
#define ARGLEN 256

enum proc_state {
    CLOSE,
    READY,
    RUNNING,
    PENDING
};

struct ProcContext {
    uint64_t rax, rcx, rdx, rbx;
    uint64_t rdi, rsi, rbp, zero;
    uint64_t r8, r9, r10, r11;
    uint64_t r12, r13, r14, r15;
    uint64_t vecnum, errno;
    uint64_t rip, cs;
    uint64_t rflags;
    uint64_t rsp, ss;
};

struct Proc {
    pgtbl_t pgtbl,
            p_pgtbl;// p_pgtbl's corresponding mapping should be
                    // different with pgtbl only at W flag, and
                    // kernel space in p_pgtbl may be not mapped,
                    // because we don't use them in p_pgtbl
    uint64_t pid;
    enum proc_state state;
    uint64_t exec_time;
    struct ProcContext context;
    struct file_desc fdesc[64];
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