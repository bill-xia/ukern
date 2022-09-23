#ifndef CONSOLE_H
#define CONSOLE_H

#include "types.h"

#define COLOR_FORE_WHITE 0x0700
#define COLOR_BACK_BLACK 0x0000
#define COLS 80
#define ROWS 25
#define TAB_SIZE 4

void console_putch(char);

#endif