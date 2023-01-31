#include "usyscall.h"
#include "stdio.h"
#include "errno.h"
#include "string.h"

char *argv[16];
char argv_buf[16][256];
char history[100][4096]; // save 100 history
char cmdbuf[4096];
int histptr[100];
int total;

int
main()
{
	int	c,
		ptr = 0,
		latest = 0, // most up-to-date history index
		hist = 0; // current working history
	for (int i = 0; i < 16; ++i) {
		argv[i] = argv_buf[i];
	}
	printf("sh # ");
	while ((c = sys_getch())) {
		if (c < 0) {
			printf("sh sys_getch() failed: %e\n", c);
			sys_exit(c);
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
			int argc = 0;
			for (int i = 0, j = 0; i <= ptr; ++i) {
				if (i < ptr && cmdbuf[i] != ' ' && cmdbuf[i] != '\t') {
					argv[argc][j] = cmdbuf[i];
					++j;
					if (j == 256) {
						printf("sh: argument too long.\n");
						goto next_buf;
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
			if (argc == 0) {
				goto new_prompt;
			}
			int child;
			if ((child = sys_fork())) {
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
		next_buf:
			strncpy(history[latest], cmdbuf, ptr);
			histptr[latest] = ptr;
			latest = (latest + 1) % 100;
			hist = latest;
			total++;
		new_prompt:
			ptr = 0;
			cmdbuf[0] = '\0';
			printf("sh # ");
			break;
		case 0xE2:
			// key up
			if (hist == (latest + 1) % 100 || (hist == 0 && total < 100)) {
				// max history
				break;
			}
			if (hist == latest) {
				// go to latest history, save current line first
				strncpy(history[latest], cmdbuf, 4096);
				histptr[latest] = ptr;
			}
			for (int i = 0; i < ptr; ++i) {
				printf("\b");
			}
			hist = (hist + 99) % 100;
			ptr = histptr[hist];
			strncpy(cmdbuf, history[hist], 4096);
			printf("%s", cmdbuf);
			break;
		case 0xE3:
			// key down
			if (hist == latest) {
				// already at current line
				break;
			}
			for (int i = 0; i < ptr; ++i) {
				printf("\b");
			}
			hist = (hist + 1) % 100;
			ptr = histptr[hist];
			strncpy(cmdbuf, history[hist], 4096);
			printf("%s", cmdbuf);
			break;
		default:
			if (ptr < 4095) {
				cmdbuf[ptr++] = c;
				cmdbuf[ptr] = '\0';
				sys_putch(c);
			} else {
				// line too long, ignore
			}
			break;
		}
	}
	return 0;
}