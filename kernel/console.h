#ifndef CONSOLE_H
#define CONSOLE_H

#include "types.h"

#define TAB_SIZE 4

struct screen {
	u32	*buf;
	u8	*glyph;
	u64	buf_siz;
	u32	width,
		height,
		pixels_per_scan_line,
		hpixel,
		vpixel,
		bytes_per_glyph;
};

void console_putch(char c);
void init_console(struct screen *screen);
extern u32*	pixelbuf;

#endif