#include "printk.h"
#include "stdarg.h"
#include "printfmt.h"
#include "console.h"

void
printk(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vprintfmt(console_putch, fmt, ap);
	va_end(ap);
}

void
panic(const char *fmt, ...)
{
	printk("panic: ");
	va_list ap;
	va_start(ap, fmt);
	vprintfmt(console_putch, fmt, ap);
	va_end(ap);
	while (1);
}