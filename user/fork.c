#include "stdio.h"
#include "usyscall.h"

int main()
{
	printf("before fork\n");
	int child;
	if ((child = sys_fork())) {
		printf("parent after fork, child: %d\n", child);
		for (int i = 0; i < 100000000; ++i)
			;
		printf("parent waiting...\n");
		sys_wait(NULL);
	} else {
		printf("child after fork\n");
	}
	sys_exit(0);
}