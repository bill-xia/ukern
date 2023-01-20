#include "pcie/pcie.h"
#include "pcie/sata.h"
#include "mem.h"
#include "printk.h"

volatile uint32_t *sata_regs;

void
pcie_sata_register(void)
{
    struct pcie_dev_type *dev_type = &pcie_dev_type_list[n_pcie_dev_type++];
    dev_type->class = 0x01;
    dev_type->subclass = 0x06;
    dev_type->progif = 0x01;
    dev_type->dev_init = pcie_sata_ahci_init;
}

void
pcie_sata_ahci_init(struct pci_config_device *cfg)
{
    int i, j;

    struct pcie_dev *dev = &pcie_dev_list[n_pcie_dev++];
    dev->class = 0x01;
    dev->subclass = 0x06;
    dev->progif = 0x01;
    dev->cfg = cfg;
    dev->vendor = pcie_readw(cfg, VENDORID);
    dev->devid = pcie_readw(cfg, DEVICEID);
    printk("found sata controller.\n");
    // AHCI Spec 1.3.1 Section 10.1
    printk("ACHI Base Address: %x\n", cfg->BAR5);
    map_mmio(k_pgtbl, KMMIO | cfg->BAR5, cfg->BAR5, NULL);
    sata_regs = (uint32_t *)(KMMIO | cfg->BAR5);
    // 1. we're aware of AHCI
    sata_regs[SATA_GHC] |= GHC_AE;
    // 2. get nports and ports mask
    int nports = (sata_regs[SATA_CAP] & CAP_NP) + 1;
    int ports_mask = sata_regs[SATA_PI];
    // 3. make sure all pors are in idle state
    struct sata_port_regs *ports = (struct sata_port_regs *)(sata_regs + (0x100 / 4));
    for (i = 0; i < 32; ++i) {
        if (!(ports_mask & (1u << i)))
            continue;
        // port enabled
        uint32_t cmd = ports[i].cmd;
        if (!(cmd & (CMD_ST | CMD_CR | CMD_FRE | CMD_FR)))
            continue;
        // port is not idle
        // 1) place into idle state with patience
        cmd &= ~CMD_ST;
        if (cmd & CMD_FRE)
            cmd &= ~CMD_FRE;
        ports[i].cmd &= cmd;
        for (j = 0; j < 500 && ports[i].cmd & (CMD_CR |CMD_FR); ++j)
            ; // wait until CR cleared
        // 2) port reset
        uint32_t sctl = ports[i].sctl;
        sctl &= ~SCTL_DET;
        sctl |= 1;
        ports[i].sctl = sctl;
    }
    // 4. get ncs
    int ncs = ((sata_regs[SATA_CAP] & CAP_NCS) >> 8) + 1;
    // 5. alloc memory for cl and fb
    for (i = 0; i < 32; ++i) {
        if (!(ports_mask & (1u << i)))
            continue;
        // port enabled
        struct PageInfo *pg;
        pg = alloc_page(FLAG_ZERO);
        ports[i].clb = (uint32_t)pg->paddr;
        ports[i].clbu = pg->paddr >> 32;
        pg = alloc_page(FLAG_ZERO);
        ports[i].fb = (uint32_t)pg->paddr;
        ports[i].fbu = pg->paddr >> 32;
    }
    // 6. clear errors
    for (i = 0; i < 32; ++i) {
        if (!(ports_mask & (1u << i)))
            continue;
        // port enabled
        ports[i].serr |= SERR_ALL;
    }
    // 7. enable interrupts
    for (i = 0; i < 32; ++i) {
        if (!(ports_mask & (1u << i)))
            continue;
        // port enabled
        // TODO: enable interrupts
    }

    printk("ACHI nports: %d, ports implemented: %x, ncs: %d\n", nports, ports_mask, ncs);
}

void
sata_read_block()
{
    // AHCI Spec 1.3.1 Section 5.5
}

void
sata_write_block()
{
    // AHCI Spec 1.3.1 Section 5.5
}