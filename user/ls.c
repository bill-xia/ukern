#include "usyscall.h"
#include "stdio.h"

struct dirent dirent;

int
main(int argc, char *argv[])
{
	if (argc > 2) {
		printf("ls: argument too much.\n"
			"usage: ls [dirname]\n");
		sys_exit(-1);
	}
	char *dirname;
	if (argc == 1)
		dirname = "/";
	else
		dirname = argv[1];
	int fd = sys_opendir(dirname);
	if (fd < 0) {
		printf("cat: %s: %e\n", dirname, fd);
		sys_exit(-2);
	}
	int r;
	while ((r = sys_readdir(fd, &dirent)) == 0) {
		if (dirent.d_ino == 0)
			break;
		printf("%c\t%s\n", dirent.d_type == DT_DIR ? 'D' : 'F', dirent.d_name);
	}
	if (r < 0) {
		printf("ls: in reading %s: %e.\n", argv[1], r);
	}
	sys_exit(0);
}