#include "usyscall.h"
#include "stdio.h"

char *argv[16];
char argv_buf[16][256];
char buf[4096];

int
main()
{
	char c;
	int ptr = 0;
	for (int i = 0; i < 16; ++i) {
		argv[i] = argv_buf[i];
	}
	printf("sh # ");
	while (c = sys_getch()) {
		if (c == -1) {
			printf("keyboard in use\n");
			while(1);
		}
		switch (c) {
		case '\b':
			if (ptr > 0) {
				ptr--;
				sys_putch('\b');
			}
			break;
		case '\n':
			// run the command
			sys_putch(c);
			buf[ptr++] = '\0';
			int argc = 0;
			for (int i = 0, j = 0; i <= ptr; ++i) {
				if (i < ptr && buf[i] != ' ' && buf[i] != '\t') {
					argv[argc][j] = buf[i];
					++j;
					if (j == 256) {
						printf("sh: argument too long.\n");
						goto clear_buf;
					}
				} else {
					if (j != 0) {
						argv[argc][j] = '\0';
						argc++;
						j = 0;
					}
					// if j == 0, there's multiple empty chars, just ignore
				}
			}
			int child;
			if (child = sys_fork()) {
				// parent
				int wstatus;
				sys_wait(&wstatus);
				if (wstatus != 0 && wstatus != 0xdeadbeef) {
					printf("sh: '%s' exited with value %d.\n",
						argv[0], wstatus);
				}
			} else {
				int r = sys_exec(argv[0], argc, argv);
				printf("sh: '%s' failed running: %e\n", argv[0], r);
				sys_exit(0xdeadbeef);
			}
		clear_buf:
			ptr = 0;
			printf("sh # ");
			break;
		default:
			if (ptr < 4095) {
				buf[ptr++] = c;
				sys_putch(c);
			} else {
				// line too long, ignore
			}
		}
	}
}