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

struct Proc {
    uint64_t pgtbl;
    uint64_t pid;
    enum proc_state state;
} *procs;

void init_proc(void);

#endif