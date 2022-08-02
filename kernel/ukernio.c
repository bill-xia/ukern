#include "ukernio.h"

static uint16_t *crt_buf = (uint16_t *)0xB8000;
static uint32_t crt_ind = 0;

void print(char *s)
{
    while ((*s) != '\0') {
        crt_buf[crt_ind] = ((uint16_t)(*s) | (COLOR_FORE_WHITE | COLOR_BACK_BLACK));
        ++crt_ind;
        ++s;
    }
}

void printint(uint64_t n)
{
    char buf[32];
    int ind = 0;
    if (n == 0) {
        buf[ind++] = '0';
        buf[ind] = 0;
        print(buf);
        return;
    }
    while (n) {
        buf[ind++] = n % 10 + '0';
        n /= 10;
    }
    for (int i = 0; i < ind - 1 - i; ++i) {
        char tmp = buf[i];
        buf[i] = buf[ind - 1 - i];
        buf[ind - 1 - i] = tmp;
    }
    buf[ind] = 0;
    print(buf);
}

void newline()
{
    while (crt_ind % 80) crt_ind++;
}

void clean()
{
    for (int i = 0; i < 80 * 25; ++i) {
        crt_buf[i] = 0;
    }
}