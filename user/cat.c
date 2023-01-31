#include "stdio.h"
#include "usyscall.h"

char buf[4097];

int main(int argc, char *argv[])
{
	if (argc != 2) {
		printf("cat: wrong argument number.\n"
			"usage: cat <filename>\n");
		sys_exit();
	}
	int fd = sys_open(argv[1]);
	if (fd < 0) {
		printf("cat: %s: %e\n", argv[1], fd);
		sys_exit();
	}
	int r;
	while ((r = sys_read(fd, buf, 4096)) > 0) {
		buf[r] = '\0';
		printf("%s", buf);
	}
	if (r < 0) {
		printf("cat: in reading %s: %e.\n", argv[1], r);
	}
	printf("\n");
	sys_exit();
}