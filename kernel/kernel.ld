OUTPUT_FORMAT("elf64-x86-64", "elf64-x86-64", "elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(prot_to_ia32e)
SECTIONS
{
    .debug_info :
        { *(.debug_info) }
    . = 0x100000;
    .text_entry : AT (0x100000)
        { obj/kernel/entry(.text .text.* .text64 .got.plt)}
    .data_entry :
        { obj/kernel/entry(.data) }
    . = 0xFFFF800000101000;
    .text : AT (0x101000)
        { EXCLUDE_FILE (obj/kernel/entry) *(.text .text.* ) *(.got.plt)}
    .data :
        { EXCLUDE_FILE (obj/kernel/entry) *(.data .data.*) }
    .bss :
        { *(.bss) }
    .rodata :
        { *(.rodata) }
    PROVIDE(end = .);
    /DISCARD/ : { *(.comment) *(.eh_frame) *(.note) *(.group)}
}