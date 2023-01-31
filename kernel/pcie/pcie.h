#ifndef PCIE_H
#define PCIE_H

#include "types.h"
#include "printk.h"

#define VENDORID    0x00
#define DEVICEID    0x02
#define COMMAND     0x04
#define STATUS      0x06
#define REVID       0x08
#define PROGIF      0x09
#define SUBCLASS    0x0A
#define CLASS       0x0B
#define CACHELINE   0x0C
#define LATENCY     0x0D
#define HDRTYPE     0x0E
#define BIST        0x0F

#define PRIBUS      0x18
#define SECBUS      0x19
#define SUBBUS      0x1A

#define PCI_CMD_ID  0x400
#define PCI_CMD_BME 0x04
#define PCI_CMD_MSE 0x02
#define PCI_CMD_IOSE    0x01

extern u64 pcie_base;

struct pci_config_hdr {
	u16	vendor_id;           /* 00 */
	u16	dev_id;
	u32	cmd_status;          /* 04 */

	u8	rev_id;              /* 08 */
	u8	prog_if;
	u8	subclass;
	u8	class;

	u8	cache_line;          /* 0C */
	u8	latency;
	u8	hdr_type : 7;
	u8	multi_func : 1;
	u8	bist;
} __attribute__((packed));

struct pci_config_bridge {
	struct pci_config_hdr	hdr;  /* 00 */
	u32	BAR0;                /* 10 */
	u32	BAR1;                /* 14 */
	u8	primary_bus;         /* 18 */
	u8	secondary_bus;
	u8	subord_bus;
	u8	timer;
	u32	pad[8];              /* 1C..3C */
} __attribute__((packed));

struct pci_config_device {
	struct pci_config_hdr	hdr;  /* 00 */
	u32	BAR0;                /* 10 */
	u32	BAR1;                /* 14 */
	u32	BAR2;                /* 18 */
	u32	BAR3;                /* 1C */
	u32	BAR4;                /* 20 */
	u32	BAR5;                /* 24 */
	u32	cardbus;             /* 28 */
	u16	subsys_vendor_id;    /* 2C */
	u16	subsys_id;
	u32	rom_addr;            /* 30 */
	u32	pad[2];              /* 34 */
	u8	irq;                 /* 3C */
	u8	irq_pin;
	u8	grant;
	u8	latency;
} __attribute__((packed));

static inline u32
pcie_read(volatile void *_base, int offset)
{
	volatile u32 *base = _base;
	return base[offset / 4];
}

static inline void
pcie_write(volatile void *_base, int offset, u32 value)
{
	volatile u32 *base = _base;
	base[offset / 4] = value;
}

static inline u16
pcie_readw(volatile void *_base, int offset)
{
	u32 r = pcie_read(_base, offset);
	offset &= 3;
	if (offset & 1) { // odd offset is not permitted
		panic("pcie_readw: odd offset %d.\n", offset);
	} else {
		r >>= 8 * (offset & 3);
	}
	return (u16)r;
}

static inline u8
pcie_readb(volatile void *_base, int offset)
{
	u32 r = pcie_read(_base, offset);
	r >>= 8 * (offset & 3);
	return (u8)r;
}

static inline u16
pcie_writew(volatile void *_base, int offset, u16 value)
{
	u32 r = pcie_read(_base, offset);
	offset &= 3;
	if (offset & 1) { // odd offset is not permitted
		panic("pcie_writew: odd offset %d.\n", offset);
	} else {
		r &= ~(0xffff << (8 * offset));
		r |= value << (8 * offset);
	}
	return (u16)r;
}

// static inline u16
// pcie_writeb(void *_base, int offset, u8 value)
// {
//     u32 r = pcie_read(_base, offset);
//     offset &= 3;
//     if (offset & 1) { // odd offset is not permitted
//         panic("pcie_writeb: odd offset %d.\n", offset);
//     } else {
//         r &= 0xffff << (8 * (2 - offset));
//         r |= value << (8 * (2 - offset));
//     }
//     return (u16)r;
// }

void init_pcie(void);

typedef void (*dev_init_fn)(volatile struct pci_config_device *pcie_dev);

struct pcie_dev_type {
	u8		class,
			subclass,
			progif;
	dev_init_fn 	dev_init;
};

struct pcie_dev {
	u8	class,
		subclass,
		progif;
	u16	vendor,
		devid;
	volatile struct pci_config_device *cfg;
};

#define NPCIEDEVTYPE 1024
#define NPCIEDEV 1024

extern int n_pcie_dev_type, n_pcie_dev;
extern struct pcie_dev_type pcie_dev_type_list[NPCIEDEVTYPE];
extern struct pcie_dev pcie_dev_list[NPCIEDEV];

#endif