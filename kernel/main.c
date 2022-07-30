#include "ukernio.h"

int main()
{
    char *s = "hello, world!";
    int i = 0;
    char *crt_buf = 0xB8000;
    while (s[i] != 0) {
        crt_buf[i * 2] = s[i];
        crt_buf[i * 2 + 1] = 0x07;
        ++i;
    }
    while (1);
}