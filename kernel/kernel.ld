SECTIONS
{
    . = 0x10000 + SIZEOF_HEADERS;
    .text : AT (512 + SIZEOF_HEADERS)
        { *(.text) }
    .data : AT (LOADADDR(.text) + SIZEOF(.text))
        { *(.data) }
    .bss :  AT (LOADADDR(.data) + SIZEOF(.data))
        { *(.bss) }
    /DISCARD/ : { *(.comment) *(.eh_frame) *(.note.gnu.property) }
}