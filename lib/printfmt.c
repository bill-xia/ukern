#include "printfmt.h"
#include "types.h"
#include "stdarg.h"
#include "errno.h"

void
printint(void (*putch)(char c), u64 n, int base, int width, char padc)
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
get_precision(u64 n, int base, int sign) {
	if (sign && (i64)n < 0) {
		n = -(i64)n;
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
printstr(void (*putch)(char), const char *str)
{
	while (*str != '\0') {
		putch(*str);
		str++;
	}
}

#define FMT_LONG 0x1

void
vprintfmt(void (*putch)(char), const char *fmt, va_list ap)
{
	u64 num;
	double flt;
	i64 sgn_num;
	int i = 0;
	while (fmt[i] != '\0') {
		if (fmt[i] == '%') {
			++i;
			u64 flags = 0;
			int width = -1;
			char padc = ' ';
		movein:
			switch (fmt[i++]) {
			case '0':
				padc = '0';
				goto movein;
				break;
			case 'l':
				flags |= FMT_LONG;
				goto movein;
				break;
			case 'c':
				num = va_arg(ap, u32);
				putch((char)(num));
				break;
			case 'd':
				if (flags & FMT_LONG)
					sgn_num = va_arg(ap, i64);
				else
					sgn_num = va_arg(ap, i32);
				if (width == -1)
					width = get_precision(sgn_num, 10, 1);
				if (sgn_num < 0) {
					putch('-');
					width--;
					sgn_num = -sgn_num;
				}
				printint(putch, sgn_num, 10, width, padc);
				break;
			case 'u':
				if (flags & FMT_LONG)
					num = va_arg(ap, u64);
				else
					num = va_arg(ap, u32);
				if (width == -1)
					width = get_precision(num, 10, 0);
				printint(putch, num, 10, width, padc);
				break;
			case 'x':
				if (flags & FMT_LONG)
					num = va_arg(ap, u64);
				else
					num = va_arg(ap, u32);
				if (width == -1)
					width = get_precision(num, 16, 0);
				printint(putch, num, 16, width, padc);
				break;
			case 'p':
				num = va_arg(ap, u64);
				if (width == -1)
					width = get_precision(num, 16, 0);
				putch('0');
				putch('x');
				printint(putch, num, 16, width, padc);
				break;
			case 's':
				num = (u64)va_arg(ap, char *);
				printstr(putch, (void *)num);
				break;
			case '%':
				putch('%');
				break;
			case 'e':
				num = -(va_arg(ap, i32));
				if (num <= 0 || num >= EMAX)
					num = 0; // wrong errno
				printstr(putch, error_msg[num]);
				break;
			case 'f':
				// float is promoted to double, so 'l' flag
				// doesn't matter
				flt = va_arg(ap, double);
				num = flt;
				if (width == -1)
					width = get_precision(num, 10, 0);
				printint(putch, num, 10, width, padc);
				flt -= num;
				num = flt * 1000000;
				if (num == 0)
					break;
				while (num % 10 == 0) {
					num /= 10;
				}
				putch('.');
				if (width == -1)
					width = get_precision(num, 10, 0);
				printint(putch, num, 10, width, padc);
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
	return;
}