#ifndef PROC_H
#define PROC_H

#include "types.h"

#define NPROCS 1024

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
} __attribute__((aligned(16)));

struct Proc {
    uint64_t pgtbl;
    uint64_t pid;
    enum proc_state state;
    struct ProcContext context;
    uint64_t rip, cs;
    uint64_t rflags;
    uint64_t rsp, ss;
} *procs, *curproc;

void init_pcb(void);
int create_proc(char *);
void run_proc(struct Proc *proc);
void kill_proc(struct Proc *proc);

#define IMAGE_SYMBOL(x) _binary_user_ ## x ## _start
#define CREATE_PROC(x) do { \
    extern char IMAGE_SYMBOL(x); \
    create_proc(&IMAGE_SYMBOL(x)); \
} while (0)

#endif