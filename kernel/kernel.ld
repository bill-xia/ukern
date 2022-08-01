OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386:x86-64)
ENTRY(prot_to_ia32e)
SECTIONS
{
    . = 0x100000;
    .text : AT (0x100000)
        { *(.text .text.* ) *(.got.plt)}
    .data : /* AT (LOADADDR(.text) + SIZEOF(.text)) */
        { *(.data) }
    .bss :  /* AT (LOADADDR(.data) + SIZEOF(.data)) */
        { *(.bss) }
    .rodata : /* AT (LOADADDR(.bss) + SIZEOF(.bss)) */
        { *(.rodata) }
    /DISCARD/ : { *(.comment) *(.eh_frame) *(.note) *(.group)}
}