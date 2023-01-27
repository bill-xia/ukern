OUTPUT_FORMAT("elf64-x86-64", "elf64-x86-64", "elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(ia32e)
SECTIONS
{
    . = 0x400000;
    .text_entry : AT (0x400000)
        { obj/kernel/entry(.text64) }
    .data_entry :
        { obj/kernel/entry(.data64) }

    . = 0xFFFF800000410000;
    .text : AT (0x410000)
        { EXCLUDE_FILE (obj/kernel/entry) *(.text .text.* ) *(.got.plt)}
    .data :
        { EXCLUDE_FILE (obj/kernel/entry) *(.data .data.*) }
    .rodata :
        { EXCLUDE_FILE (obj/kernel/entry) *(.rodata) }
    .bss :
        { *(.bss) }
    .rodata :
        { *(.rodata) }
    PROVIDE(end = .);
    /DISCARD/ : { *(.comment) *(.eh_frame) *(.note) *(.group)}
}