#include "pcie/pcie.h"
#include "pcie/sata.h"
#include "pcie/ata_cmd.h"
#include "mem.h"
#include "printk.h"
#include "errno.h"
#include "x86.h"

uint32_t *sata_regs;
static struct sata_cmd_hdr *cmd_list[32];
volatile struct sata_port_regs *ports;
int working_port;

int sata_read_block(uint64_t block);

void
pcie_sata_register(void)
{
    struct pcie_dev_type *dev_type = &pcie_dev_type_list[n_pcie_dev_type++];
    dev_type->class = 0x01;
    dev_type->subclass = 0x06;
    dev_type->progif = 0x01;
    dev_type->dev_init = pcie_sata_ahci_init;
}

int check_port(int port)
{
    uint32_t ssts = ports[port].ssts;
    uint8_t ipm = ((ssts >> 8) & 0x0F), det = (ssts & 0xF);
    if (det != 3 || ipm != 1) {
        return 0;
    }
    uint32_t sig = ports[port].sig;
    printk("port %d sig %x\n", port, sig);
    return sig != 0xeb140101 && sig != 0xc33c0101 && sig != 0x96690101;
}

void
pcie_sata_ahci_init(volatile struct pci_config_device *cfg)
{
    int i, j;

    struct pcie_dev *dev = &pcie_dev_list[n_pcie_dev++];
    dev->class = 0x01;
    dev->subclass = 0x06;
    dev->progif = 0x01;
    dev->cfg = cfg;
    dev->vendor = pcie_readw(cfg, VENDORID);
    dev->devid = pcie_readw(cfg, DEVICEID);
    // printk("found sata controller. cmd: %x, abar: %x\n",
    //     cfg->hdr.cmd_status);

    cfg->hdr.cmd_status |= PCI_CMD_BME | PCI_CMD_MSE |
        PCI_CMD_IOSE;

    // printk("ACHI Base Address: %x, cmd: %x\n", cfg->BAR5, cfg->hdr.cmd_status);
    map_mmio(k_pgtbl, KMMIO | cfg->BAR5, cfg->BAR5, NULL);
    lcr3(rcr3());
    sata_regs = (uint32_t *)(KMMIO | cfg->BAR5);

    // uint16_t cmd = pcie_readw(cfg, COMMAND);
    // cmd &= ~PCI_CMD_ID;
    // pcie_writew(cfg, COMMAND, cmd);


    // sata_regs[SATA_BOHC] |= BOHC_OOS;
    // while (sata_regs[SATA_BOHC] & BOHC_BOS) ;
    // AHCI Spec 1.3.1 Section 10.1
    // 1. we're aware of AHCI
    sata_regs[SATA_GHC] |= GHC_AE;
    // sata_regs[SATA_GHC] = GHC_RST;
    // while (sata_regs[SATA_GHC] & GHC_RST);
    // sata_regs[SATA_GHC] = GHC_AE;
    sata_regs[SATA_GHC] |= GHC_IE;
    // 2. get nports and ports mask
    int nports = (sata_regs[SATA_CAP] & CAP_NP) + 1;
    int ports_mask = sata_regs[SATA_PI];
    // 3. make sure all ports are in idle state
    // printk("IS.IPS: %x", sata_regs[SATA_IS]);
    ports = (struct sata_port_regs *)(sata_regs + (0x100 / 4));
    for (i = 0; i < 32; ++i) {
        if (!(ports_mask & (1u << i)))
            continue;
        // port enabled
        uint32_t cmd = ports[i].cmd;
        if (!check_port(i))
            continue;
        working_port = i;
        // printk("port %d cmd: %x\n", i, cmd);
        if (cmd & (CMD_ST | CMD_CR | CMD_FRE | CMD_FR)) {
            // 1) place into idle state with patience
            ports[i].cmd &= ~CMD_ST;
            if (ports[i].cmd & CMD_FRE) {
                ports[i].cmd &= ~CMD_FRE;
                while (ports[i].cmd & CMD_FR)
                    ;
            }
            for (j = 0; j < 5000 && ports[i].cmd & (CMD_CR |CMD_FR); ++j)
                ; // wait until CR cleared
            // printk("port %d cmd after reset: %x, j: %d\n", i, ports[i].cmd, j);
            // 2) port reset
            if (j == 5000) {
                // printk("port %d reset.\n", i);
                uint32_t sctl = ports[i].sctl;
                sctl &= ~SCTL_DET;
                sctl |= 1;
                ports[i].sctl = sctl;
                for (j = 0; j < 10000; ++j) ;
                sctl &= ~SCTL_DET;
                ports[i].sctl = sctl;
                while ((ports[i].sctl & SCTL_DET) != 3)
                    ;
            }
        }
        if (ports[i].cmd & (CMD_ST | CMD_CR | CMD_FRE | CMD_FR)) {
            panic("port not in idle state.\n");
        }

        // 5. alloc memory for cl and fb
        struct PageInfo *pg;
        pg = alloc_page(FLAG_ZERO);
        uint64_t paddr = pg->paddr;
        map_mmio(k_pgtbl, KMMIO | paddr, paddr, NULL);
        lcr3(rcr3());
        // cmd_list[i] = (struct sata_cmd_hdr *)(((uint64_t)ports[i].clbu << 32) | ports[i].clb);
        cmd_list[i] = (struct sata_cmd_hdr *)(KMMIO | paddr);
        // printk("old cl %lx, old fb %lx\n", ((uint64_t)ports[i].clbu << 32) | ports[i].clb,
        //     ((uint64_t)ports[i].fbu << 32) | ports[i].fb);
        ports[i].clb = (uint32_t)paddr; // 1K
        ports[i].clbu = paddr >> 32;
        paddr += (1 << 10);
        ports[i].fb = (uint32_t)paddr; // 256 B
        ports[i].fbu = paddr >> 32;
    
        // 6. clear errors
        ports[i].serr = 0xFFFFFFFF;
        ports[i].is = 0;
        ports[i].ie = 0;

        // alloc memory for cmd_tbl
        // printk("achi port functional: %d, err: %x\n", i, ports[i].serr);

        ports[i].ci = 0;

        ports[i].cmd |= CMD_FRE;
        for (int j = 0; j < 100000; ++j) ;
        // printk("ci: %x\n", ports[i].ci);
        // printk("functional: tfd %x, ssts %x\n", ports[i].tfd, ports[i].ssts);
        ports[i].cmd |= CMD_ST;

        // 7. enable interrupts
        ports[i].is = 0;
        ports[i].ie = 0xFFFFFFFF;
        // printk("ie: %x\n", ports[i].ie);
    }
    // 4. get ncs
    int ncs = ((sata_regs[SATA_CAP] & CAP_NCS) >> 8) + 1;

    printk("ACHI nports: %d, ports implemented: %x, ncs: %d\n", nports, ports_mask, ncs);

    walk_pgtbl(k_pgtbl, KDISK | 0x0000, NULL, 1);
    lcr3(rcr3());

    sata_regs[SATA_GHC] |= GHC_IE;

    sata_read_block(0);
    for (i = 0; i < 1000000; ++i) ;
    uint8_t *buf = (uint8_t *)(KDISK | 0x0000);
    char *hex = "0123456789ABCDEF";
    for (i = 0; i < 64; ++i) {
        uint8_t lo = buf[i] & 0xf, hi = buf[i] >> 4;
        printk("%c%c ", hex[hi], hex[lo]);
        if (i % 16 == 15)
            printk("\n");
    }
}

static uint64_t
blk2kaddr(int blk)
{
    return KDISK | (blk << PGSHIFT);
}

int
sata_read_block(uint64_t block)
{
    // AHCI Spec 1.3.1 Section 5.5
    int i, port = working_port, slot = 0;
    ports[port].is = 0xFFFF;
    // printk("port is: %x\n", ports[port].is);
    for (slot = 0; slot < 32; ++slot) {
        if ((ports[port].ci & (1u << slot)) || (ports[port].sact & (1u << slot)))
            continue;
        break;
    }
    if (slot == 32) {
        panic("no slot.\n");
        return -E_NOSLOT;
    }
    struct sata_cmd_hdr *cmd_hdr = &cmd_list[port][slot];
    int prdtl = 1, cfl = sizeof(struct reg_h2d_fis) / 4, isatapi = 0;
    // build ATA command
    struct PageInfo *pg = alloc_page(FLAG_ZERO);
    struct sata_cmd_tbl *cmd_tbl = (struct sata_cmd_tbl *)P2K(pg->paddr);
    cmd_hdr->dw2 = pg->paddr;
    cmd_hdr->dw3 = pg->paddr >> 32;

    // pte_t *ppte;
    // walk_pgtbl(k_pgtbl, cmd_tbl, &ppte, 0);
    // printk("cmd_tbl: %p, pte %lx\n", cmd_tbl, *ppte);
    // cmd_tbl->cfis
    struct reg_h2d_fis *cfis = &cmd_tbl->cfis.reg_h2d;
    cfis->fis_type = FIS_TYPE_REG_H2D;
    cfis->c = 1;
    cfis->command = ATA_COMMAND_READ_DMA_EXT;

    uint64_t lba = block * 8;
    cfis->lba0 = (uint8_t)lba;
    cfis->lba1 = (uint8_t)(lba >> 8);
    cfis->lba2 = (uint8_t)(lba >> 16);
    cfis->device = 1<<6;

    cfis->lba3 = (uint8_t)(lba >> 24);
    cfis->lba4 = (uint8_t)(lba >> 32);
    cfis->lba5 = (uint8_t)(lba >> 40);

    cfis->countl = 8; // 8 sector, 1 4K-block
    cfis->counth = 0;
    // cmd_tbl->acmd
    // cmd_tbl->prdt
    for (i = 0; i < prdtl; ++i) {
        uint64_t vaddr = blk2kaddr(block);
        pte_t *pte;
        walk_pgtbl(k_pgtbl, vaddr, &pte, 0);
        uint64_t paddr = PAGEADDR(*pte);
        // printk("tbl: %p, prdt: %p\n", cmd_tbl, cmd_tbl->prdt);
        cmd_tbl->prdt[i].dba = (uint32_t)paddr;
        cmd_tbl->prdt[i].dbau = paddr >> 32;
        cmd_tbl->prdt[i].dw3 = (PGSIZE - 1);
    }

    // set cmd_hdr
    uint32_t dw0 = (prdtl << CMDHDR_PRDTL_SHIFT) | cfl;
    if (isatapi)
        dw0 |= CMDHDR_A;
    cmd_hdr[slot].dw0 = dw0;
    cmd_hdr[slot].dw1 = 0;

    // run!
    ports[port].ci |= (1u << slot);
    while (1)
    {
        // if (ports[port].tfd & TFD_BSY)
        //     printk("busy");
        if ((ports[port].ci & (1 << slot)) == 0)
            break;
        if (ports[port].is & (1 << 30))
            panic("read disk error.\n");
    }
    while (ports[port].ci != 0) {
        printk("[R]");
    }
    printk("sata_read(): port: %d, slot: %d, prdbc: %d\n", port, slot, cmd_hdr[slot].dw1);
    return 0;
}

int
sata_write_block()
{
    // AHCI Spec 1.3.1 Section 5.5
    // TODO
    return 0;
}