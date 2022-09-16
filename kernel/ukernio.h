#ifndef UKERNIO_H
#define UKERNIO_H

#include "types.h"
#include "stdarg.h"

#define COLOR_FORE_WHITE 0x0700
#define COLOR_BACK_BLACK 0x0000

void printk(char *fmt, ...);
void putchar(char c);

#endif