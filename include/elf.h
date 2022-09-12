typedef unsigned Elf32_Addr;
typedef unsigned short Elf32_Half;
typedef unsigned Elf32_Off;
typedef int Elf32_Sword;
typedef unsigned Elf32_Word;

#define EI_NIDENT 16
#define ELF_MAGIC 0x464C457FU

struct Elf32_Ehdr {
    unsigned char e_ident[EI_NIDENT]; // 0-15
    Elf32_Half e_type; // 16-17
    Elf32_Half e_machine; // 18-19
    Elf32_Word e_version; // 20-23
    Elf32_Addr e_entry; // 24-27
    Elf32_Off e_phoff; // 28-31
    Elf32_Off e_shoff; // 32-35
    Elf32_Word e_flags; // 36-39
    Elf32_Half e_ehsize; // 40-41
    Elf32_Half e_phentsize; // 42-43
    Elf32_Half e_phnum; // 44-45
    Elf32_Half e_shentsize; // 46-47
    Elf32_Half e_shnum; // 48-49
    Elf32_Half e_shstrndx; // 50-51
};

#define PT_NULL 0
#define PT_LOAD 1
#define PT_DYNAMIC 2
#define PT_INTERP 3
#define PT_NOTE 4
#define PT_SHLIB 5
#define PT_PHDR 6
#define PT_LOPROC 0x70000000
#define PT_HIPROC 0x7fffffff

struct Elf32_Phdr {
    Elf32_Word p_type;
    Elf32_Off p_offset;
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    Elf32_Word p_filesz;
    Elf32_Word p_memsz;
    Elf32_Word p_flags;
    Elf32_Word p_align;
};