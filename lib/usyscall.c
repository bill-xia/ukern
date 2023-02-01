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
sys_exit(int exit_val)
{
	syscall(SYS_exit, exit_val, 0, 0, 0, 0, 0);
}

int
sys_fork(void)
{
	return syscall(SYS_fork, 0, 0, 0, 0, 0, 0);
}

int
sys_exec(const char *fn, int argc, char *argv[])
{
	return syscall(SYS_exec, (u64)fn, (u64)argc, (u64)argv, 0, 0, 0);
}

int
sys_wait(int *wstatus)
{
	return syscall(SYS_wait, (u64)wstatus, 0, 0, 0, 0, 0);
}

int
sys_open(const char *fn)
{
	return syscall(SYS_open, (u64)fn, 0, 0, 0, 0, 0);
}

int
sys_read(int fd, char *buf, u32 sz)
{
	return syscall(SYS_read, (u64)fd, (u64)buf, sz, 0, 0, 0);
}

int
sys_write(int fd, const char *buf, u32 sz)
{
	return syscall(SYS_write, (u64)fd, (u64)buf, sz, 0, 0, 0);
}

int
sys_close(int fd)
{
	return syscall(SYS_close, (u64)fd, 0, 0, 0, 0, 0);
}

int
sys_opendir(const char *fn)
{
	return syscall(SYS_opendir, (u64)fn, 0, 0, 0, 0, 0);
}

int
sys_readdir(int fd, struct dirent *buf)
{
	return syscall(SYS_readdir, (u64)fd, (u64)buf, 0, 0, 0, 0);
}

int
sys_chdir(const char *fn)
{
	return syscall(SYS_chdir, (u64)fn, 0, 0, 0, 0, 0);
}

int
sys_pipe(int fd[2])
{
	return syscall(SYS_pipe, (u64)fd, 0, 0, 0, 0, 0);
}
