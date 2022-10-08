OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
SECTIONS
{
  . = 0x7c00;
  .text : { *(.text) }
  .data : { *(.data) }
  /DISCARD/ : { *(.comment) *(.eh_frame) *(.note.gnu.property) *(.bss)}
  . = 0x7dfe;
  .mbrsign : { obj/boot/boot.out(.mbrsign)}
}