#ifndef SYSCALL_H
#define SYSCALL_H

#include "types.h"
#include "proc.h"

#define E_NOMEM 1

void syscall(struct ProcContext *tf);

#endif