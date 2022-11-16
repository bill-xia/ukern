#include "stdio.h"
#include "usyscall.h"

char buf[512];

int main()
{
    int fd = sys_open("/helloworld");
    printf("opened file /helloworld: %d\n", fd);
    if (fd < 0) {
        sys_exit();
    }
    int r = sys_read(fd, buf, 511);
    // r = sys_read(fd, buf, 511);
    printf("sys_read(%d, buf, 512): %d\n", fd, r);
    buf[r] = 0;
    for (int i = 0; i < r; ++i) {
        sys_putch(buf[i]);
    }
    sys_exit();
}