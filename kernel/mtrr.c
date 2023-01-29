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
		printk("cr0 old caching strategy: %lx\n", (cr0 & (CR0_CD | CR0_NW)) >> 29);
		cr0 &= ~(CR0_CD | CR0_NW);
		lcr0(cr0);
	}
	// detect MTRR
	u64 mtrr_cap = rdmsr(MSR_MTRR_CAP);
	if (!(mtrr_cap & MTRR_CAP_WC)) {
		panic("Doesn't support Write Combing.\n");
	} else {
		printk("Support Write Combing.\n");
	}

	// disable
	deftype = rdmsr(MSR_MTRR_DEF_TYPE);
	if (deftype & (MTRR_DEF_TYPE_FE | MTRR_DEF_TYPE_E)) {
		printk("MTRR enabled, disbling.\n");
	}
	deftype &= ~(MTRR_DEF_TYPE_E | MTRR_DEF_TYPE_FE);
	stmsr(MSR_MTRR_DEF_TYPE, deftype);

	// set pixelbuf to Write-Combining
	int	i,
		mtrr_vcnt = mtrr_cap & MTRR_CAP_VCNT_MASK;
	stmsr(MSR_MTRR_PHYS_BASE(0), K2P(PAGEADDR((u64)pixelbuf)) | MTRR_TYPE_WC);
	stmsr(MSR_MTRR_PHYS_MASK(0), (0xFFul << 28) | MTRR_MASK_V); // TODO: get MAXPHYSADDR and mask according to it
	for (i = 1; i < mtrr_vcnt; ++i) {
		stmsr(MSR_MTRR_PHYS_BASE(i), 0);
		stmsr(MSR_MTRR_PHYS_MASK(i), 0); // without MTRR_MASK_V, disable
	}

	// enabling
	deftype = rdmsr(MSR_MTRR_DEF_TYPE);
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