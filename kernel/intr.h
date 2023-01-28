#ifndef INTR_H
#define INTR_H

#include "types.h"

#define NIDT	72

struct IDTGateDesc {
	u16	offset1;
	u16	seg_sel;
	u8	ist:3, zeros:5;
	u8	type:4, S:1, DPL:2, P:1;
	u16	offset2;
	u32	offset3;
	u32	reserved;
};

struct IDTDesc{
	u16 limit;
	u64 addr;
} __attribute__ ((packed));

void init_intr(void);

#endif