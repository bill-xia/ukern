#ifndef PRINTK_H
#define PRINTK_H

#include "stdarg.h"
#include "printfmt.h"

void printk(const char *fmt, ...);
void panic(const char *fmt, ...);

#endif