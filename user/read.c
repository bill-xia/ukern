#include "stdio.h"
#include "usyscall.h"

char buf[4097];

int main()
{
    int fd = sys_open("/helloworld");
    printf("opened file /helloworld: %d\n", fd);
    if (fd < 0) {
        sys_exit();
    }
    int r = sys_read(fd, buf, 4096);
    printf("sys_read(%d, buf, 4096): %d\n", fd, r);
    buf[r] = 0;
    if (r >= 0) {
        buf[r] = 0;
        for (int i = 0; i < r; ++i) {
            sys_putch(buf[i]);
        }
    } else {
        printf("sys_read() failed.\n");
    }
    sys_exit();
}