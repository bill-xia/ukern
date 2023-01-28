#include "x86.h"
#include "pic.h"

int
init_pic(void)
{
	/* ICW1 */
	outb(0x20, 0x11); /* Master port A */
	outb(0xA0, 0x11); /* Slave port A */

	/* ICW2 */
	outb(0x21, 0x30); /* Master offset of 0x30 in the IDT */
	outb(0xA1, 0x38); /* Master offset of 0x38 in the IDT */

	/* ICW3 */
	outb(0x21, 0x04); /* Slaves attached to IR line 2 */
	outb(0xA1, 0x02); /* This slave in IR line 2 of master */

	/* ICW4 */
	outb(0x21, 0x05); /* Set as master */
	outb(0xA1, 0x01); /* Set as slave */

	disable_pic();

	return 0;
}

int disable_pic(void)
{
	outb(0x20, 0x20); /* master EOI */
	outb(0xA0, 0x20); /* slave EOI */
	outb(0x21, 0xff); /* master PIC */
	outb(0xA1, 0xff); /* slave PIC */
	return 0;
}