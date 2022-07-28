SECTIONS
{
  . = 0x7c00;
  .text : { *(.text) }
  .data : { *(.data) }
  .bss : { *(.bss) }
  /DISCARD/ : { *(.comment) *(.eh_frame) *(.note.gnu.property) }
  . = 0x7dfe;
  .mbrsign : { boot/boot.o(.mbrsign)}
}