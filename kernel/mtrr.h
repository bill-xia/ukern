#ifndef MTRR_H
#define MTRR_H

#include "x86.h"

#define CR0_CD (1 << 30)
#define CR0_NW (1 << 29)

#define MSR_MTRR_CAP		0xFE
#define MSR_MTRR_DEF_TYPE	0x2FF
#define MSR_MTRR_PHYS_BASE(i)	(0x200 + 2 * (i))
#define MSR_MTRR_PHYS_MASK(i)	(0x201 + 2 * (i))

#define MTRR_CAP_VCNT_MASK	0xFF
#define MTRR_CAP_FIX		0x100
#define MTRR_CAP_WC		0x400
#define MTRR_CAP_SMRR		0x800

#define MTRR_TYPE_UC		0x00
#define MTRR_TYPE_WC		0x01
#define MTRR_TYPE_WB		0x06
#define MTRR_MASK_V		0x800

#define MTRR_DEF_TYPE_FE	(1ul << 10)
#define MTRR_DEF_TYPE_E		(1ul << 11)

int init_mtrr(void);

#endif