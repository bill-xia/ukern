#ifndef PRINTK_H
#define PRINTK_H

#include "stdarg.h"

#define assert(x) { if (!(x)) { printk("%s:%d: assert %s failed.\n", __FILE__, __LINE__, #x); while(1); } }

void printk(const char *fmt, ...);
void panic(const char *fmt, ...);

#endif