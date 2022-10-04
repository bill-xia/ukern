#ifndef CONSOLE_H
#define CONSOLE_H

#include "types.h"

#define COLOR_WHITE_ON_BLACK 0x0700
#define COLOR_BLACK_ON_WHITE 0xF000
#define COLS 80
#define ROWS 25
#define TAB_SIZE 4

void console_putch(char c);
void init_console(void);

#endif