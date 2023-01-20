#ifndef PCIE_H
#define PCIE_H

#include "types.h"

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

extern uint64_t pcie_base;

struct pci_config_hdr {
    uint16_t    vendor_id;           /* 00 */
    uint16_t    dev_id;
    uint32_t    cmd_status;          /* 04 */

    uint8_t     rev_id;              /* 08 */
    uint8_t     prog_if;
    uint8_t     subclass;
    uint8_t     class;

    uint8_t     cache_line;          /* 0C */
    uint8_t     latency;
    uint8_t     hdr_type : 7;
    uint8_t     multi_func : 1;
    uint8_t     bist;
} __attribute__((packed));

struct pci_config_bridge {
    struct pci_config_hdr hdr;  /* 00 */
    uint32_t   BAR0;                /* 10 */
    uint32_t   BAR1;                /* 14 */
    uint8_t  primary_bus;         /* 18 */
    uint8_t  secondary_bus;
    uint8_t  subord_bus;
    uint8_t  timer;
    uint32_t   pad[8];              /* 1C..3C */
} __attribute__((packed));

struct pci_config_device {
    struct pci_config_hdr hdr;  /* 00 */
    uint32_t   BAR0;                /* 10 */
    uint32_t   BAR1;                /* 14 */
    uint32_t   BAR2;                /* 18 */
    uint32_t   BAR3;                /* 1C */
    uint32_t   BAR4;                /* 20 */
    uint32_t   BAR5;                /* 24 */
    uint32_t   cardbus;             /* 28 */
    uint16_t subsys_vendor_id;    /* 2C */
    uint16_t subsys_id;
    uint32_t   rom_addr;            /* 30 */
    uint32_t   pad[2];              /* 34 */
    uint8_t  irq;                 /* 3C */
    uint8_t  irq_pin;
    uint8_t  grant;
    uint8_t  latency;
} __attribute__((packed));

static inline uint32_t
pcie_read(void *_base, int offset)
{
    uint32_t *base = _base;
    return base[offset / 4];
}

static inline void
pcie_write(void *_base, int offset, uint32_t value)
{
    uint32_t *base = _base;
    base[offset / 4] = value;
}

static inline uint16_t
pcie_readw(void *_base, int offset)
{
    uint32_t r = pcie_read(_base, offset);
    offset &= 3;
    if (offset & 1) { // odd offset is not permitted
        panic("pcie_readw: odd offset %d.\n", offset);
    } else {
        r >>= 8 * (offset & 3);
    }
    return (uint16_t)r;
}

static inline uint8_t
pcie_readb(void *_base, int offset)
{
    uint32_t r = pcie_read(_base, offset);
    r >>= 8 * (offset & 3);
    return (uint8_t)r;
}

void init_pcie(void);

typedef void (*dev_init_fn)(struct pci_config_device *pcie_dev);

struct pcie_dev_type {
    uint8_t class, subclass, progif;
    dev_init_fn dev_init;
};

struct pcie_dev {
    uint8_t class, subclass, progif;
    uint16_t vendor, devid;
    struct pci_config_device *cfg;
};

#define NPCIEDEVTYPE 1024
#define NPCIEDEV 1024

extern int n_pcie_dev_type, n_pcie_dev;
extern struct pcie_dev_type pcie_dev_type_list[NPCIEDEVTYPE];
extern struct pcie_dev_type pcie_dev_list[NPCIEDEV];

#endif