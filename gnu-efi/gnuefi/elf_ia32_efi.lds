OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)
SECTIONS
{
  . = 0;
  ImageBase = .;
  /* .hash and/or .gnu.hash MUST come first! */
  .hash : { *(.hash) }
  .gnu.hash : { *(.gnu.hash) }
  . = ALIGN(4096);
  .text :
  {
   _text = .;
   *(.text)
   *(.text.*)
   *(.gnu.linkonce.t.*)
   . = ALIGN(16);
  }
  _etext = .;
  _text_size = . - _text;
  . = DATA_SEGMENT_ALIGN (CONSTANT (MAXPAGESIZE), CONSTANT (COMMONPAGESIZE));
  .sdata :
  {
   _data = .;
   *(.got.plt)
   *(.got)
   *(.srodata)
   *(.sdata)
   *(.sbss)
   *(.scommon)
  }
  . = ALIGN(4096);
  .data :
  {
   *(.rodata*)
   *(.data)
   *(.data1)
   *(.data.*)
   *(.sdata)
   *(.got.plt)
   *(.got)
   /* the EFI loader doesn't seem to like a .bss section, so we stick
      it all into .data: */
   *(.sbss)
   *(.scommon)
   *(.dynbss)
   *(.bss)
   *(COMMON)
  }
  .note.gnu.build-id : { *(.note.gnu.build-id) }

  . = ALIGN(4096);
  .dynamic  : { *(.dynamic) }
  . = ALIGN(4096);
  .rel :
  {
    *(.rel.data)
    *(.rel.data.*)
    *(.rel.got)
    *(.rel.stab)
    *(.data.rel.ro.local)
    *(.data.rel.local)
    *(.data.rel.ro)
    *(.data.rel*)
  }
  _edata = .;
  _data_size = . - _etext;
  . = ALIGN(4096);
  .reloc :		/* This is the PECOFF .reloc section! */
  {
    *(.reloc)
  }
  . = ALIGN(4096);
  .dynsym   : { *(.dynsym) }
  . = ALIGN(4096);
  .dynstr   : { *(.dynstr) }
  . = ALIGN(4096);
  . = DATA_SEGMENT_END (.);
  /DISCARD/ :
  {
    *(.rel.reloc)
    *(.eh_frame)
    *(.note.GNU-stack)
  }
  .comment 0 : { *(.comment) }
}
