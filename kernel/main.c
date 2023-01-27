#include "init.h"
#include "printk.h"
#include "intr.h"
#include "proc.h"
#include "sched.h"
#include "errno.h"
#include "mem.h"

int main(struct boot_args *args)
{
	init(args);
	// CREATE_PROC(hello);
	// CREATE_PROC(sort);
	// CREATE_PROC(divzero);
	// // CREATE_PROC(fork);
	CREATE_PROC(idle);
	// CREATE_PROC(read);
	// CREATE_PROC(mal_read);
	// // CREATE_PROC(exec);
	// CREATE_PROC(forkexec);
	CREATE_PROC(sh);
	sched();
	while (1);
}