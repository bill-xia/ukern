OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
SECTIONS
{
  . = 0x7c00;
  .text : { *(.text) }
  .data : { *(.data) }
  .bss : { *(.bss) }
  /DISCARD/ : { *(.comment) *(.eh_frame) *(.note.gnu.property) }
  . = 0x7dfe;
  .mbrsign : { boot/boot.out(.mbrsign)}
}