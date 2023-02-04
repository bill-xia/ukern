#include "stdio.h"
#include "usyscall.h"

int main()
{
	int *end_umem = sys_sbrk(0);
	int ch = sys_fork();
	if (ch == 0) { // child
		printf("Child visiting after end_umem, this should fail.\n");
		printf("value: %ld\n", *(end_umem + 1024));
		printf("Malicious access success! Haha!\n");
	} else { // parent
		sys_sbrk(4096);
		printf("Parent visiting after end_umem, this should success.\n");
		printf("value: %ld\n", *(end_umem + 1024));
		printf("Parent accessed successfully.\n");
	}
	sys_exit(0);
}