OUTPUT_FORMAT("elf64-tradlittlemips", "elf64-tradbigmips", "elf64-tradlittlemips")
OUTPUT_ARCH(mips)
ENTRY(_start)
SECTIONS
{
  .text 0x0 : {
    _text = .;
    *(.text.head)
    *(.text)
    *(.text.*)
    *(.gnu.linkonce.t.*)
    *(.srodata)
    *(.rodata*)
    . = ALIGN(16);
  }
  _etext = .;
  _text_size = . - _text;
  . = DATA_SEGMENT_ALIGN (CONSTANT (MAXPAGESIZE), CONSTANT (COMMONPAGESIZE));
  .dynamic  : { *(.dynamic) }
  .data :
  {
   _data = .;
   *(.sdata)
   *(.data)
   *(.data1)
   *(.data.*)
   *(.got.plt)
   HIDDEN (_gp = ALIGN (16) + 0x7ff0);
   *(.got)

   /* the EFI loader doesn't seem to like a .bss section, so we stick
      it all into .data: */
   . = ALIGN(16);
   _bss = .;
   *(.sbss)
   *(.scommon)
   *(.dynbss)
   *(.bss)
   *(COMMON)
   . = ALIGN(16);
   _bss_end = .;
  }

  .rel.dyn : { *(.rel.dyn) }
  .rel.plt : { *(.rel.plt) }
  .rel.got : { *(.rel.got) }
  .rel.data : { *(.rel.data) *(.rel.data*) }
  _edata = .;
  _data_size = . - _etext;

  . = ALIGN(4096);
  .dynsym   : { *(.dynsym) }
  . = ALIGN(4096);
  .dynstr   : { *(.dynstr) }
  . = ALIGN(4096);
  .note.gnu.build-id : { *(.note.gnu.build-id) }
  . = DATA_SEGMENT_END (.);
  /DISCARD/ :
  {
    *(.rel.reloc)
    *(.eh_frame)
    *(.MIPS.abiflags)
    *(.note.GNU-stack)
  }
  .comment 0 : { *(.comment) }
}
