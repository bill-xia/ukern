#include "x86.h"
#include "sse.h"
#include "printk.h"

int
init_sse()
{
	u64 rax, rbx, rcx, rdx;
	cpuid(1, &rax, &rbx, &rcx, &rdx);
	if (!(rdx & (1 << 24))) {
		panic("Your CPU doesn't suport FXSAVE and FXRSTOR instructions.\n");
	}
	if (!(rdx & (1 << 25))) {
		panic("Your CPU doesn't support SSE.\n");
	}
	if (!(rdx & (1 << 26))) {
		panic("Your CPU doesn't support SSE2.\n");
	}
	// if (!(rcx & (1 << 9))) {
	// 	panic("Your CPU doesn't support SSE3.\n");
	// }
	u64 cr0 = rcr0();
	printk("cr0: %lx\n", cr0);
	cr0 &= ~CR0_EM;
	cr0 |= CR0_MP;
	lcr0(cr0);
	u64 cr4 = rcr4();
	printk("cr4: %lx\n", cr4);
	cr4 |= CR4_OSFXSR | CR4_OSXMMEXCPT;
	lcr4(cr4);
	return 0;
}