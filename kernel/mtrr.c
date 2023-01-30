#include "mtrr.h"
#include "console.h"
#include "mem.h"
#include "printk.h"

int
init_mtrr()
{
	int deftype;
	u64 cr0 = rcr0();
	if (cr0 & (CR0_CD | CR0_NW)) {
		printk("cr0 old caching strategy: %lx\n",
			(cr0 & (CR0_CD | CR0_NW)) >> 29);
		cr0 &= ~(CR0_CD | CR0_NW);
		lcr0(cr0);
	}
	// detect MTRR
	u64 mtrr_cap = rdmsr(MSR_MTRR_CAP);
	if (!(mtrr_cap & MTRR_CAP_WC)) {
		panic("Doesn't support Write Combing.\n");
	}

	// disable
	deftype = rdmsr(MSR_MTRR_DEF_TYPE);
	// if (deftype & (MTRR_DEF_TYPE_FE | MTRR_DEF_TYPE_E)) {
	// 	printk("MTRR enabled, disbling.\n");
	// }
	deftype &= ~(MTRR_DEF_TYPE_E | MTRR_DEF_TYPE_FE);
	stmsr(MSR_MTRR_DEF_TYPE, deftype);

	// set pixelbuf to Write-Combining
	int mtrr_vcnt = mtrr_cap & MTRR_CAP_VCNT_MASK;
	stmsr(MSR_MTRR_PHYS_BASE(0), K2P(PAGEADDR(pixelbuf)) | MTRR_TYPE_WC);

	// get MAXPHYSADDR and mask according to it
	u64 addr_size;
	cpuid(0x80000008, &addr_size, NULL, NULL, NULL);
	u64	phy_shift = addr_size & CPUID_ADDR_PHYSHIFT_MASK,
		lin_shift = (addr_size & CPUID_ADDR_LINSHIFT_MASK) >> 8;
	assert(lin_shift >= 48); // thus our memory layout works
	stmsr(MSR_MTRR_PHYS_MASK(0), (~MASK(23)&MASK(phy_shift)) | MTRR_MASK_V);

	for (int i = 1; i < mtrr_vcnt; ++i) {
		stmsr(MSR_MTRR_PHYS_BASE(i), 0);
		// without MTRR_MASK_V, thus disabled
		stmsr(MSR_MTRR_PHYS_MASK(i), 0);
	}

	// enable
	deftype = (MTRR_DEF_TYPE_E | MTRR_DEF_TYPE_FE) | MTRR_TYPE_WB;
	stmsr(MSR_MTRR_DEF_TYPE, deftype);

	// test our screen again, it should be fast!
	// int	color,
	// 	ind;
	// for (color = 0; color < 0x1000000; color += 0x10101) {
	// 	for (ind = 0; ind < 1920 * 1080; ++ind) {
	// 		pixelbuf[ind] = color;
	// 	}
	// }

	// for (ind = 0; ind < 1920 * 1080; ++ind) {
	// 	pixelbuf[ind] = 0;
	// }
}