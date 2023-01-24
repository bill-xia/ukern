#include "x86.h"
#include "pic.h"

int
init_pic(void)
{
    /* ICW1 */
    outb( 0x11, 0x20 ); /* Master port A */
    outb( 0x11, 0xA0 ); /* Slave port A */

    /* ICW2 */
    outb( 0x20, 0x21 ); /* Master offset of 0x20 in the IDT */
    outb( 0x28, 0xA1 ); /* Master offset of 0x28 in the IDT */

    /* ICW3 */
    outb( 0x04, 0x21 ); /* Slaves attached to IR line 2 */
    outb( 0x02, 0xA1 ); /* This slave in IR line 2 of master */

    /* ICW4 */
    outb( 0x05, 0x21 ); /* Set as master */
    outb( 0x01, 0xA1 ); /* Set as slave */

    disable_pic();

    return 0;
}

int disable_pic(void)
{
    outb( 0xff, 0x21 ); /* master PIC */
    outb( 0xff, 0xA1 ); /* slave PIC */
    return 0;
}