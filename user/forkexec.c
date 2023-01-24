#include "stdio.h"
#include "usyscall.h"

char *argv[16] = {
	"hello",
	"arg"
};

int main()
{
	printf("before fork\n");
	int child;
	if (child = sys_fork()) {
		printf("parent after fork, child: %d\n", child);
	} else {
		printf("child after fork\n");
		printf("before exec\n");
		sys_exec("/hello", 2, argv);
		printf("after exec\n");
		sys_exit();
	}
	sys_exit();
}