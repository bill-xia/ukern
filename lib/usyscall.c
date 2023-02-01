#include "usyscall.h"

int
syscall(int num, u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, u64 a6)
{
	int ret;
	asm volatile (
		"int $32"
		:"=a" (ret):
		"a" (num),
		"d" (a1),
		"c" (a2),
		"b" (a3),
		"D" (a4),
		"S" (a5): "cc", "memory"
	);
	return ret;
}

void
sys_hello(void)
{
	syscall(2, 0, 0, 0, 0, 0, 0);
}

void
sys_putch(char c)
{
	syscall(3, c, 0,0,0,0,0);
}

void
sys_exit(int exit_val)
{
	syscall(1,exit_val,0,0,0,0,0);
}

int
sys_fork(void)
{
	return syscall(4,0,0,0,0,0,0);
}

int
sys_open(const char *fn)
{
	return syscall(5, (u64)fn, 0,0,0,0,0);
}

int
sys_read(int fd, char *fn, u32 sz)
{
	return syscall(6, (u64)fd, (u64)fn, sz, 0,0,0);
}

int
sys_exec(const char *fn, int argc, char *argv[])
{
	return syscall(7, (u64)fn, (u64)argc, (u64)argv, 0,0,0);
}

int
sys_getch(void)
{
	return syscall(8, 0,0,0,0,0,0);
}

int
sys_wait(int *wstatus)
{
	return syscall(9, (u64)wstatus,0,0,0,0,0);
}

int
sys_opendir(const char *fn)
{
	return syscall(10, (u64)fn,0,0,0,0,0);
}

int
sys_readdir(int fd, struct dirent *buf)
{
	return syscall(11, (u64)fd, (u64)buf,0,0,0,0);
}

int
sys_chdir(const char *fn)
{
	return syscall(12, (u64)fn, 0,0,0,0,0);
}