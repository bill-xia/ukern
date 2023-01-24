#include "stdio.h"
#include "usyscall.h"
#include "printfmt.h"
#include "stdarg.h"

void
printf(const char *fmt, ...)
{
	va_list(ap);
	va_start(ap, fmt);
	vprintfmt(sys_putch, fmt, ap);
	va_end(ap);
}