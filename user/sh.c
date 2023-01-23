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
            buf[ptr++] = ' ';
            int argc = 0;
            for (int i = 0, j = 0; i < ptr; ++i) {
                if (buf[i] != ' ' && buf[i] != '\t') {
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
                // TODO: sys_wait()
                for (int i = 0; i < 100000000; ++i) ;
            } else {
                // printf("exec(\"%s\", %d, %p)\n", argv[0], argc, argv);
                sys_exec(argv[0], argc, argv);
                sys_exit();
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