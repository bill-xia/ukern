#ifndef PCI_H
#define PCI_H

#include "x86.h"

#define CONFIG_ADDRESS 0xCF8
#define CONFIG_DATA 0xCFC

#define VENDOR 0

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
    if (offset & 3) {
        r >>= 16;
    } else {
        r &= (1 << 16) - 1;
    }
    return (uint16_t)r;
}

#endif