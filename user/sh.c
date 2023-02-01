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
char path[4096];
char kbd_buf[4096];

int
main()
{
	int	kbd_head = 0,
		kbd_len = 0,
		c,
		ptr = 0,
		latest = 0, // most up-to-date history index
		hist = 0; // current working history
	for (int i = 0; i < 16; ++i) {
		argv[i] = argv_buf[i];
	}
	printf("sh # ");
	while (1) {
		if (kbd_len == 0) {
			kbd_len = sys_read(0, kbd_buf, 4096);
			if (kbd_len < 0) {
				printf("sh: sys_read() failed: %e\n", kbd_len);
				sys_exit(kbd_len);
			}
			if (kbd_len == 0) {
				break;
			}
			kbd_head = 0;
		}
		// consume char
		c = (u8)kbd_buf[kbd_head++];
		if (kbd_head == 4096)
			kbd_head = 0;
		kbd_len--;

		switch (c) {
		case '\b':
			if (ptr > 0) {
				ptr--;
				printf("\b");
			}
			break;
		case '\n':
			// run the command
			printf("%c", c);
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
			if (argc == 2 && strcmp(argv[0], "cd", 256) == 0) {
				int r = sys_chdir(argv[1]);
				if (r < 0) {
					printf("cd %s failed: %e\n", argv[1], r);
				}
				goto next_buf;
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
				path[0] = '/';
				path[1] = '\0';
				strcat(path, argv[0]);
				int r = sys_exec(path, argc, argv);
				printf("sh: '%s' failed running: %e\n", path, r);
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
				printf("%c", c);
			} else {
				// line too long, ignore
			}
			break;
		}
	}
	sys_exit(0);
}