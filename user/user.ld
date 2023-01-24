OUTPUT_FORMAT("elf64-x86-64", "elf64-x86-64", "elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(main)
SECTIONS
{
	. = 0x400000;
	.text :
		{ *(.text .text.* ) *(.got.plt)}
	.data :
		{ *(.data .data.*) }
	.bss :
		{ *(.bss) }
	.rodata :
		{ *(.rodata) }
	/DISCARD/ : { *(.comment) *(.eh_frame) *(.note) *(.group)}
}