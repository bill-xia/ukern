#ifndef UKERN_H
#define UKERN_H

// just syscall numbers
enum {
	SYS_exit,
	SYS_fork,
	SYS_exec,
	SYS_wait,
	SYS_open,
	SYS_read,
	SYS_write,
	SYS_close,
	SYS_opendir,
	SYS_readdir,
	SYS_chdir,
	SYS_pipe,
};

#endif