#include "stdio.h"
#include "usyscall.h"
#include "printfmt.h"
#include "stdarg.h"

static char buf[4096];
static int len;

void
putch(char c)
{
	buf[len++] = c;
	buf[len] = '\0';
}

void
printf(const char *fmt, ...)
{
	va_list(ap);
	va_start(ap, fmt);
	vprintfmt(putch, fmt, ap);
	va_end(ap);
	sys_write(1, buf, len);
	len = 0;
}