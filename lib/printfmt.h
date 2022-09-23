#ifndef PRINTFMT_H
#define PRINTFMT_H

#include "stdarg.h"

void printfmt(void (*putch)(char), const char *fmt, ...);
void vprintfmt(void (*putch)(char), const char *fmt, va_list ap);

#endif