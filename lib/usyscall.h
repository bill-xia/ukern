#ifndef SYSCALL_H
#define SYSCALL_H

#include "types.h"
#include "dirent.h"

int syscall(int num, u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6);

void sys_hello(void);
void sys_putch(char);
void sys_exit(int exit_val);
int sys_fork(void);
int sys_open(const char *fn);
int sys_read(int fd, char *fn, u32 sz);
int sys_exec(const char *fn, int argc, char *argv[]);
int sys_getch(void);
int sys_wait(int *wstatus);
int sys_opendir(const char *fn);
int sys_readdir(int fd, struct dirent *buf);
int sys_chdir(const char *fn);
#endif