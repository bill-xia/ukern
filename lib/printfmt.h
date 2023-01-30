#ifndef PRINTFMT_H
#define PRINTFMT_H

#include "stdarg.h"

#define assert(x) { if (!(x)) { printk("%s:%d: assert %s failed.\n", __FILE__, __LINE__, #x); while(1); } }

void printfmt(void (*putch)(char), const char *fmt, ...);
void vprintfmt(void (*putch)(char), const char *fmt, va_list ap);

#endif