#include "ukernio.h"
#include "stdarg.h"

static uint16_t *crt_buf = (uint16_t *)0xB8000;
static uint32_t crt_ind = 0;

void printstr(char *s)
{
    while ((*s) != '\0') {
        crt_buf[crt_ind] = ((uint16_t)(*s) | (COLOR_FORE_WHITE | COLOR_BACK_BLACK));
        ++crt_ind;
        ++s;
    }
}

void printint(uint64_t n, int base, int width, char padc)
{
    static char buf[32];
    int len = 0;
    if (n == 0) {
        buf[len++] = '0';
    } else {
        while (n) {
            buf[len++] = n % base + '0';
            if (n % base > 9) {
                buf[len - 1] = n % base - 10 + 'a';
            }
            n /= base;
        }
        for (int i = 0; i < len - 1 - i; ++i) {
            char tmp = buf[i];
            buf[i] = buf[len - 1 - i];
            buf[len - 1 - i] = tmp;
        }
    }
    buf[len] = 0;
    for (int i = 0; i < width - len; ++i) {
        putchar(padc);
    }
    for (int i = 0; i < len; ++i) {
        putchar(buf[i]);
    }
}

void putchar(char c)
{
    crt_buf[crt_ind] = (c | (COLOR_FORE_WHITE | COLOR_BACK_BLACK));
    ++crt_ind;
}

int get_precision(uint64_t n, int base, int sign) {
    if (sign && (int64_t)n < 0) {
        n = -(int64_t)n;
    }
    int len = 0;
    if (n == 0) return 1;
    while (n) {
        n /= base;
        ++len;
    }
    return len;
}

void printf(char *fmt, ...)
{
    va_list ap;
	va_start(ap, fmt);
	
    uint64_t num;
    int i = 0;
    while (fmt[i] != '\0') {
        if (fmt[i] == '%') {
            ++i;
            uint64_t flags = 0;
            int width = -1, precision = -1;
            char padc = ' ';
            switch (fmt[i++]) {
            case 'd':
                num = va_arg(ap, int64_t);
                if (width == -1) width = get_precision(num, 10, 1);
                if (num < 0) {
                    putchar('-');
                    width--;
                }
                printint(num, 10, width, padc);
                break;
            case 'x':
                num = va_arg(ap, uint64_t);
                if (width == -1) width = get_precision(num, 16, 0);
                printint(num, 16, width, padc);
                break;
            case 'p': ;
                num = va_arg(ap, uint64_t);
                if (width == -1) width = get_precision(num, 16, 0);
                putchar('0');
                putchar('x');
                width -= 2;
                printint(num, 16, width, padc);
                break;
            case '%':
                putchar('%');
                break;
            default:
                putchar('%');
                putchar(fmt[i - 1]);
                break;
            }
        } else if (fmt[i] == '\n') {
            do {
                putchar(' ');
            } while (crt_ind % 80);
            ++i;
        } else {
            putchar(fmt[i]);
            ++i;
        }
    }
	va_end(ap);
}