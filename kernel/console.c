#include "console.h"
#include "mem.h"
#include "printk.h"

static u32 ind = 0, rows = 0, cols = 0,
	height, width,
	hpixel = 0, vpixel = 0, bytes_per_glyph;
static u16 color = COLOR_WHITE_ON_BLACK;

static char cbuf[1920 * 1080];
u32 *pixelbuf;
static u8 *font;

int
glyph_at(char ch, int i, int j)
{
	int	n = i * ((hpixel + 7) / 8) * 8 + j,
		ind = n / 8,
		off = n % 8;
	return (font[ch * bytes_per_glyph + ind] >> (7 - off)) & 1;
}

void
print_font_at(char ch, int pos)
{
	cbuf[pos] = ch;
	int r = (pos / cols) * vpixel, c = (pos % cols) * hpixel;
	for (int i = 0; i < vpixel; ++i) {
		for (int j = 0; j < hpixel; ++j) {
			if (glyph_at(ch, i, j))
				pixelbuf[(r + i) * width + c + j] = 0x00FFFFFF;
			else
				pixelbuf[(r + i) * width + c + j] = 0x0;
		}
	}
}

void
console_putch(char c)
{
	if (ind == rows * cols) {
		// screen full
		for (int i = 0; i < (rows - 1) * cols; ++i) {
			print_font_at(cbuf[i + cols], i);
		}
		for (int i = (rows - 1) * cols; i < rows * cols; ++i) {
			print_font_at(' ', i);
		}
		ind -= cols;
	}
	switch (c) {
	case '\n':
		do {
			print_font_at(' ', ind);
			++ind;
		} while (ind % cols);
		break;
	case '\b':
		ind--;
		print_font_at(' ', ind);
		break;
	case '\r':
		ind -= ind % cols;
		break;
	case '\t':
		do {
			print_font_at(' ', ind);
			++ind;
		} while (ind % TAB_SIZE);
		break;
	default:
		print_font_at(c, ind);
		++ind;
		break;
	}
}

void
init_console(struct screen *screen)
{
	vpixel = screen->vpixel;
	height = screen->height;
	rows = height / vpixel;
	hpixel = screen->hpixel;
	width = screen->width;
	cols = width / hpixel;
	pixelbuf = (u32 *)P2K(screen->buf);
	bytes_per_glyph = screen->bytes_per_glyph;
	font = (u8 *)P2K(screen->glyph);
	for (int i = 0; i < rows; ++i) {
		console_putch('\n');
	}
	ind = 0;

	// We assert buf is (1<<23) byte aligned, and we'll mark
	// [buf, buf+(1<<23)] as cache-combined to spped up graphic,
	// see mtrr.c for details.
	assert(((u64)screen->buf & MASK(23)) == 0);
}