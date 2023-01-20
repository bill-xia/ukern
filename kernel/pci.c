#include "pci.h"
#include "printk.h"
#include "mem.h"
#include "pcie/xhci.h"

void
check_device(uint8_t bus, uint8_t device)
{
    uint8_t func = 0;
    uint16_t vendor = pci_readw(bus, device, func, VENDORID);
    if (vendor != 0xFFFF) {
        // printk("available pci device %d:%d\n", bus, device);
        uint32_t class = pci_read(bus, device, func, 0x8) >> 8;
        // printk("class code: %x\n", class);
        if (class == 0x0c0330) { // XHCI
            init_xhci(bus, device, func);
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