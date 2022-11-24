#include "pci.h"
#include "printk.h"
#include "mem.h"

void
check_device(uint8_t bus, uint8_t device)
{
    uint8_t func = 0;
    uint16_t vendor = pci_readw(bus, device, func, VENDOR);
    if (vendor != 0xFFFF) {
        printk("available pci device %d:%d\n", bus, device);
        uint32_t class = pci_read(bus, device, func, 0x8) >> 8;
        printk("class code: %x\n", class);
        if (class == 0x0c0330) { // XHCI
            uint64_t bar = pci_read(bus, device, func, 0x10);
            bar |= pci_read(bus, device, func, 0x14) << 32;
            printk("BAR: %lx\n", bar);
            pte_t *pte;
            map_mmio(k_pml4, bar, bar, &pte);
            *pte |= PTE_PWT | PTE_PCD;
            uint32_t *xhci_root = (uint32_t *)(bar & 0xFFFFFFFFFFFFFFF0);
            printk("xhci_root: %p\n", xhci_root);
            printk("struct param #1: %x\n", xhci_root[1]);
        }
    }
}

void
init_pci()
{
    uint16_t bus;
    uint8_t device;
    for (bus = 0; bus < 256; ++bus) {
        for (device = 0; device < 64; ++device) {
            check_device(bus, device);
        }
    }
}