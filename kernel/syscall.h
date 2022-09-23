#ifndef SYSCALL_H
#define SYSCALL_H

#include "types.h"
#include "proc.h"

void syscall(struct ProcContext *tf);
void sys_hello(void);

#endif