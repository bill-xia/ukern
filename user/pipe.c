#include "usyscall.h"
#include "stdio.h"

char buf[1000];
const char msg[] = "hello, child!";

int main()
{
	int fd[2], r;
	if ((r = sys_pipe(fd)) < 0) {
		printf("sys_pipe failed: %e\n", r);
		sys_exit(r);
	}
	if (!(fd[0] >= 2 && fd[1] >= 2)) {
		printf("assert(fd[0] >= 2 && fd[1] >= 2) failed.\n");
		sys_exit(-1);
	}
	int ch = sys_fork();
	if (ch < 0) {
		printf("fork failed: %e\n", ch);
		sys_exit(ch);
	}
	if (ch > 0) {
		// parent
		sys_close(fd[0]);
		for (int i = 0; i < 1000; ++i) {
			sys_write(fd[1], msg, sizeof(msg) - 1);
		}
		sys_close(fd[1]);
		sys_wait(NULL);
		sys_exit(0);
	} else {
		// child
		sys_close(fd[1]);
		int r, tmp = 3;
		for (int i = 0; i < 10000000; ++i) {
			tmp = tmp * i / 2;
		}
		while ((r = sys_read(fd[0], buf, 999)) > 0) {
			buf[r] = '\0';
			printf("%d, %s", tmp, buf);
		}
		if (r < 0) {
			printf("child sys_read failed: %e\n", r);
		}
		printf("\n");
		sys_close(fd[0]);
		sys_exit(0);
	}
}