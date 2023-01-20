#include "pcie/xhci.h"
#include "pci.h"
#include "mem.h"
#include "printk.h"

uint32_t *cap_base, *op_base, *dcbaap, *cmd_ring;
struct msix_table_entry *msix_table;
uint64_t *msix_pba;

uint8_t
xhci_readb(void *base, uint32_t offset)
{
    uint32_t *lbase = base;
    uint32_t val = lbase[offset / 4];
    val >>= 8 * (offset & 3);
    return val & 0xFF;
}

uint32_t
xhci_readl(void *base, uint32_t offset)
{
    uint32_t *lbase = base;
    uint32_t val = lbase[offset / 4];
    return val;
}

void
xhci_writel(void *base, uint32_t offset, uint32_t val)
{
    uint32_t *lbase = base;
    lbase[offset / 4] = val;
}

uint8_t
xhci_writeb(void *base, uint32_t offset, uint8_t val)
{
    uint32_t *lbase = base;
    uint32_t mask = ~(0xFF << (offset & 3)),
        newval = (lbase[offset / 4] & mask);
    newval |= (val << (offset & 3));
    lbase[offset / 4] = newval;
}

void
init_xhci(uint8_t bus, uint8_t device, uint8_t func)
{
    /** print Base Address Registers **/
    uint64_t bar;
    for (int i = 0; i < 6; ++i) {
        bar = pci_read(bus, device, func, 0x10 + 4 * i);
        printk("BAR%d: %x\n", i, bar);
    }

    uint32_t msix_table_siz = 0xFFFFFFFF;
    /** print capcabilities **/
    if (pci_readw(bus, device, func, STATUS) & 0x10) {
        // capability pointer enabled
        uint32_t cap = pci_readb(bus, device, func, CAPPTR);
        while (cap) {
            uint32_t cap_struct = pci_read(bus, device, func, cap);
            printk("cap %x: %x\n", cap, cap_struct);
            if ((cap_struct & 0xFF) == 0x11) { // MSI-X
                /** MSI-X table **/
                msix_table_siz = cap_struct >> 16;
                uint32_t table_offset = pci_read(bus, device, func, cap + 4);
                printk("table_offset: %p\n", table_offset);
                uint32_t table_bir = table_offset & 0x7; table_offset &= 0xFFFFFFF8;
                bar = pci_read(bus, device, func, 0x10 + 4 * table_bir);
                uint64_t msix_table_addr = (bar & 0xFFFFFFF0) + table_offset;
                if ((bar & 0x6) == 4) {
                    msix_table_addr |= (uint64_t)pci_read(bus, device, func, 0x14 + 4 * table_bir) << 32;
                }
                pte_t *pte;
                walk_pgtbl(k_pgtbl, KMMIO | msix_table_addr, &pte, 0);
                if (pte == NULL) { // not mapped
                    map_mmio(k_pgtbl, KMMIO | msix_table_addr, msix_table_addr, &pte);
                }
                msix_table = (struct msix_table_entry *)(KMMIO | msix_table_addr);
                /** MSI-X PBA **/
                uint32_t pba_offset = pci_read(bus, device, func, cap + 8);
                printk("pba_offset: %p\n", pba_offset);
                uint32_t pba_bir = pba_offset & 0x7; pba_offset &= 0xFFFFFFF8;
                bar = pci_read(bus, device, func, 0x10 + 4 * pba_bir);
                uint64_t msix_pba_addr = (bar & 0xFFFFFFF0) + pba_offset;
                if ((bar & 0x6) == 4) {
                    msix_pba_addr |= (uint64_t)pci_read(bus, device, func, 0x14 + 4 * pba_bir) << 32;
                }
                walk_pgtbl(k_pgtbl, KMMIO | msix_pba_addr, &pte, 0);
                if (pte == NULL) { // not mapped
                    map_mmio(k_pgtbl, KMMIO | msix_pba_addr, msix_pba_addr, &pte);
                }
                msix_pba = (uint64_t *)(KMMIO | msix_pba_addr);
            }
            cap = (cap_struct & 0xFF00) >> 8;
        }
    } else {
        printk("Capability pointer disabled.\n");
        while (1);
    }
    bar = pci_read(bus, device, func, 0x10);
    bar |= (uint64_t)pci_read(bus, device, func, 0x14) << 32;
    printk("BAR: %lx\n", bar);

    /** MMIO **/
    pte_t *pte;
    map_mmio(k_pgtbl, KMMIO + bar, bar, &pte);
    uint32_t *xhci_root = (uint32_t *)((KMMIO + bar) & 0xFFFFFFFFFFFFFFF0);
    
    /** start initialize **/
    printk("xhci_root: %p\n", xhci_root);
    printk("struct param #1: %x\n", xhci_root[1]);
    cap_base = KMMIO + bar;
    op_base = cap_base + xhci_readb(cap_base, CAPLENGTH) / 4;
    int polls = 0;
    while (xhci_readl(op_base, USBSTS) & CNR)
        polls++;
    printk("Controller Ready after %d polls\n", polls);
    // max_slots_en
    uint8_t max_slots = xhci_readb(cap_base, HCSPARAMS1);
    xhci_writeb(op_base, CONFIG, max_slots);
    // dcbaap
    struct PageInfo *pginfo = alloc_page(FLAG_ZERO);
    dcbaap = (uint32_t *)pginfo->paddr;
    xhci_writel(op_base, DCBAAP, pginfo->paddr);
    xhci_writel(op_base, DCBAAP + 4, pginfo->paddr >> 32);
    // command ring
    pginfo = alloc_page(FLAG_ZERO);
    cmd_ring = (uint32_t *)pginfo->paddr;
    xhci_writel(op_base, CRCR, pginfo->paddr);
    xhci_writel(op_base, CRCR + 4, pginfo->paddr >> 32);
    if (msix_table_siz == 0xFFFFFFFF) {
        printk("panic: This PC doesn't support MSI-X.\n");
        while (1);
    } else if (msix_table_siz >= 256) {
        printk("panic: MSI-X table size too large to fit in one 4K page: %d.\n", msix_table_siz + 1);
        while (1);
    }
    printk("MSI-X table size: %d\n", msix_table_siz + 1);
    printk("MSI_X table: %p\n", msix_table);
    printk("MSI_X PBA: %p\n", msix_pba);
    
}