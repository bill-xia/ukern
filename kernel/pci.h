#ifndef PCI_H
#define PCI_H

#include "x86.h"

#define CONFIG_ADDRESS 0xCF8
#define CONFIG_DATA 0xCFC

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

#define CAPPTR   0x34

static inline uint32_t
pci_read(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
    uint32_t addr,
        lbus = bus,
        lslot = slot,
        lfunc = func,
        loffset = offset;
    addr = (lbus << 16) | (lslot << 11) | (lfunc << 8) | (offset & 0xFC) | (0x80000000);
    outl(CONFIG_ADDRESS, addr);
    return inl(CONFIG_DATA);
}

static inline void
pci_write(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value)
{
    uint32_t addr,
        lbus = bus,
        lslot = slot,
        lfunc = func,
        loffset = offset;
    addr = (lbus << 16) | (lslot << 11) | (lfunc << 8) | (offset & 0xFC) | (0x80000000);
    outl(CONFIG_ADDRESS, addr);
    outl(CONFIG_DATA, value);
}

static inline uint16_t
pci_readw(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
    uint32_t addr,
        lbus = bus,
        lslot = slot,
        lfunc = func,
        loffset = offset;
    addr = (lbus << 16) | (lslot << 11) | (lfunc << 8) | (offset & 0xFC) | (0x80000000);
    outl(CONFIG_ADDRESS, addr);
    uint32_t r = inl(CONFIG_DATA);
    if (offset & 3) { // odd offset is not permitted
        r >>= 16;
    } else {
        r &= (1 << 16) - 1;
    }
    return (uint16_t)r;
}

static inline uint8_t
pci_readb(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
    uint32_t addr,
        lbus = bus,
        lslot = slot,
        lfunc = func,
        loffset = offset;
    addr = (lbus << 16) | (lslot << 11) | (lfunc << 8) | (offset & 0xFC) | (0x80000000);
    outl(CONFIG_ADDRESS, addr);
    uint32_t r = inl(CONFIG_DATA);
    if (offset & 3) {
        r >>= 8 * (offset & 3);
    }
    r &= 0xFF;
    return (uint8_t)r;
}

#endif