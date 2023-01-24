#ifndef XHCI_H
#define XHCI_H

#include "types.h"

// Capability Registers
#define CAPLENGTH	0x00
#define HCIVERSION	0x02
#define HCSPARAMS1	0x04
#define HCSPARAMS2	0x08
#define HCSPARAMS3	0x0C
#define HCCPARAMS1	0x10
#define DBOFF		0x14
#define RTSOFF		0x18
#define HCCPARAMS2	0x1C

// Operational Registers
#define USBCMD		0x00
#define USBSTS		0x04
	#define CNR		0x800
#define PAGESIZE	0x08
#define DNCTRL		0x14
#define CRCR		0x18
#define DCBAAP		0x30
#define CONFIG		0x38

extern u32 *cap_base, *op_base;

struct msix_table_entry {
	u32	msg_addr,
		msg_upper_addr,
		msg_data,
		vec_control;
} __attribute__((packed));

void init_xhci(u8 bus, u8 device, u8 func);

#endif