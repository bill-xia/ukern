#include "elf.h"
#include "types.h"
#include "x86.h"

#define SECTSIZE 512

void
wait_disk(void)
{
	// wait for disk ready
	while ((inb(0x1F7) & 0xC0) != 0x40)
		/* do nothing */;
}

void
read_sect(uint8_t *dst, uint32_t offset)
{
	// wait for disk to be ready
	wait_disk();

	outb(0x1F2, 1);		// count = 1
	outb(0x1F3, offset);
	outb(0x1F4, offset >> 8);
	outb(0x1F5, offset >> 16);
	outb(0x1F6, (offset >> 24) | 0xE0);
	outb(0x1F7, 0x20);	// cmd 0x20 - read sectors

	// wait for disk to be ready
	wait_disk();

	// read a sector
	insl(0x1F0, dst, SECTSIZE/4);
}

void read_seg(uint8_t *p_addr, int length, uint32_t offset)
{
    offset = offset / SECTSIZE + 1;
    uint8_t *p_addr_end = p_addr + length;
    p_addr = (uint32_t)p_addr & ~(SECTSIZE - 1);
    while (p_addr < p_addr_end) {
        read_sect(p_addr, offset);
        p_addr = p_addr + SECTSIZE;
        offset++;
    }
}

int c_entry()
{
    char *loader_img = 0x100000;
    read_sect(loader_img, 1);
    struct Elf32_Ehdr *elf_hdr = 0x100000;
    if (*(uint32_t *)(elf_hdr->e_ident) != ELF_MAGIC) {
        return -1;
    }
    struct Elf32_Phdr *prog_hdrs = loader_img + sizeof(struct Elf32_Ehdr);
    for (int i = 0; i < elf_hdr->e_phnum; ++i) {
        // if (prog_hdrs[i].p_type == PT_LOAD) {
            read_seg(prog_hdrs[i].p_paddr, prog_hdrs[i].p_filesz, prog_hdrs[i].p_vaddr);
            int j = 0;
            for (j = prog_hdrs[i].p_filesz; j < prog_hdrs[i].p_memsz; ++j) {
                *(char *)(prog_hdrs[i].p_vaddr + j) = 0;
            }
        // }
    }
    ((void (*)(void)) (elf_hdr->e_entry))();
    return 0;
}