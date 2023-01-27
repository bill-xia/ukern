#include "mem.h"
#include "proc.h"
#include "intr.h"
#include "sched.h"
#include "console.h"
#include "lapic.h"
#include "ioapic.h"
#include "fs/fs.h"
#include "acpi.h"
#include "pic.h"
#include "fs/disk.h"

void init(struct screen *screen)
{
	end_kmem = end;
	init_console(screen);
	printk("Hello, world!\n");
	while (1);
	init_gdt();
	init_kpageinfo(); // after the kernel image, is the k_pageinfo array
	init_kpgtbl(); // then comes kernel pagetable, which maps the whole physical memory space
	init_pcb(); // then Process Control Blocks
	init_intr();
	init_freepages(); // initialize the free page list
	// *(int*)0x7FFFFFFF0000 = 0;
	// from now on, anyone needing a new page have to call kalloc()
	init_mp();
	init_pic();
	init_ioapic();
	// init_fs();
	init_acpi();
	init_disk();
}