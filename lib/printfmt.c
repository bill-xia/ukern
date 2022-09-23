#include "printfmt.h"
#include "types.h"
#include "stdarg.h"

void
printint(void (*putch)(char c), uint64_t n, int base, int width, char padc)
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
        putch(padc);
    }
    for (int i = 0; i < len; ++i) {
        putch(buf[i]);
    }
}

int
get_precision(uint64_t n, int base, int sign) {
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

void
printfmt(void (*putch)(char), const char *fmt, ...)
{
    va_list ap;
	va_start(ap, fmt);
    vprintfmt(putch, fmt, ap);
    va_end(ap);
}

void
vprintfmt(void (*putch)(char), const char *fmt, va_list ap)
{
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
                    putch('-');
                    width--;
                }
                printint(putch, num, 10, width, padc);
                break;
            case 'x':
                num = va_arg(ap, uint64_t);
                if (width == -1) width = get_precision(num, 16, 0);
                printint(putch, num, 16, width, padc);
                break;
            case 'p': ;
                num = va_arg(ap, uint64_t);
                if (width == -1) width = get_precision(num, 16, 0);
                putch('0');
                putch('x');
                printint(putch, num, 16, width, padc);
                break;
            case '%':
                putch('%');
                break;
            default:
                putch('%');
                putch(fmt[i - 1]);
                break;
            }
        } else {
            putch(fmt[i]);
            ++i;
        }
    }
	va_end(ap);
}