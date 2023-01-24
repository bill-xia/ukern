#include "kbd.h"
#include "x86.h"
#include "printk.h"

char kbd_buffer[4096];
int kbd_buf_beg = 0, kbd_buf_siz = 0;

int
kbd_getch(void)
{
	int c;
	uint8_t stat, data;
	static uint32_t shift;

	stat = inb(KBSTATP);
    // printk("stat: %x ", (uint32_t)stat);
	if ((stat & KBS_DIB) == 0)
		return -1;
	// Ignore data from mouse.
	if (stat & KBS_TERR)
		return -1;

	data = inb(KBDATAP);
    // printk("data: %x ", (uint32_t)data);

	if (data == 0xE0) {
		// E0 escape character
		shift |= E0ESC;
		return 0;
	} else if (data & 0x80) {
		// Key released
		data = (shift & E0ESC ? data : data & 0x7F);
		shift &= ~(shiftcode[data] | E0ESC);
		return 0;
	} else if (shift & E0ESC) {
		// Last character was an E0 escape; or with 0x80
		data |= 0x80;
		shift &= ~E0ESC;
	}

	shift |= shiftcode[data];
	shift ^= togglecode[data];

	c = charcode[shift & (CTL | SHIFT)][data];
	if (shift & CAPSLOCK) {
		if ('a' <= c && c <= 'z')
			c += 'A' - 'a';
		else if ('A' <= c && c <= 'Z')
			c += 'a' - 'A';
	}

	// Process special keys
	// Ctrl-Alt-Del: reboot
	if (!(~shift & (CTL | ALT)) && c == KEY_DEL) {
		printk("Rebooting!\n");
		outb(0x92, 0x3); // courtesy of Chris Frost
	}

    // printk("char: %x\n", (uint32_t)c);
	return c;
}