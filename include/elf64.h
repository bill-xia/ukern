#ifndef ELF64_H
#define ELF64_H

typedef unsigned long	Elf64_Addr;
typedef unsigned short	Elf64_Half;
typedef unsigned long	Elf64_Off;
typedef int 		Elf64_Sword;
typedef unsigned	Elf64_Word;
typedef long		Elf64_Slword;
typedef unsigned long	Elf64_Lword;

#define EI_NIDENT	16
#define ELF_MAGIC	0x464C457FU

struct Elf64_Ehdr {
	unsigned char	e_ident[EI_NIDENT]; // 0-15
	Elf64_Half	e_type; // 16-17
	Elf64_Half	e_machine; // 18-19
	Elf64_Word	e_version; // 20-23
	Elf64_Addr	e_entry; // 24-31
	Elf64_Off	e_phoff; // 32-39
	Elf64_Off	e_shoff; // 40-47
	Elf64_Word	e_flags; // 48-51
	Elf64_Half	e_ehsize; // 52-53
	Elf64_Half	e_phentsize; // 54-55
	Elf64_Half	e_phnum; // 56-57
	Elf64_Half	e_shentsize; // 58-59
	Elf64_Half	e_shnum; // 60-61
	Elf64_Half	e_shstrndx; // 62-63
};

#define PT_NULL		0
#define PT_LOAD 	1
#define PT_DYNAMIC	2
#define PT_INTERP	3
#define PT_NOTE		4
#define PT_SHLIB	5
#define PT_PHDR		6
#define PT_LOPROC	0x70000000
#define PT_HIPROC	0x7fffffff

struct Elf64_Phdr {
	Elf64_Word p_type; // 0-3
	Elf64_Word p_flags; // 4-7
	Elf64_Off  p_offset; // 8-15
	Elf64_Addr p_vaddr; // 16-23
	Elf64_Addr p_paddr; // 24-31
	Elf64_Lword p_filesz; // 32-39
	Elf64_Lword p_memsz; // 40-47
	Elf64_Lword p_align; // 48-56
};

#endif