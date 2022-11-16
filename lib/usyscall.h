#ifndef SYSCALL_H
#define SYSCALL_H

#include "types.h"

int syscall(int num, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5, uint64_t a6);

void sys_hello(void);
void sys_putch(char);
void sys_exit(void);
int sys_fork(void);
int sys_open(const char *fn);
int sys_read(int fd, char *fn, uint32_t sz);
int sys_exec(const char *fn, int argc, char *argv[]);

#endif