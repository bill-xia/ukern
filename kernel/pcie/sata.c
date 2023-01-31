#include "pcie/pcie.h"
#include "pcie/sata.h"
#include "pcie/ata_cmd.h"
#include "fs/disk.h"
#include "mem.h"
#include "printk.h"
#include "errno.h"
#include "x86.h"
#include "fs/fs.h"

volatile u32 *sata_regs;
static struct sata_cmd_hdr *cmd_list[32];
volatile struct sata_port_regs *ports;

void
pcie_sata_register(void)
{
	struct pcie_dev_type *dev_type = &pcie_dev_type_list[n_pcie_dev_type++];
	dev_type->class = 0x01;
	dev_type->subclass = 0x06;
	dev_type->progif = 0x01;
	dev_type->dev_init = pcie_sata_ahci_init;
}

int
check_port(int port)
{
	u32 ssts = ports[port].ssts;
	u8 ipm = ((ssts >> 8) & 0x0F), det = (ssts & 0xF);
	if (det != 3 || ipm != 1) {
		return 0;
	}
	u32 sig = ports[port].sig;
	// printk("port %d sig %x\n", port, sig);
	return sig != 0xeb140101 && sig != 0xc33c0101 && sig != 0x96690101;
}

void
pcie_sata_ahci_init(volatile struct pci_config_device *cfg)
{
	struct pcie_dev *dev = &pcie_dev_list[n_pcie_dev++];
	dev->class = 0x01;
	dev->subclass = 0x06;
	dev->progif = 0x01;
	dev->cfg = cfg;
	dev->vendor = pcie_readw(cfg, VENDORID);
	dev->devid = pcie_readw(cfg, DEVICEID);
	printk("Found sata controller.\n");

	cfg->hdr.cmd_status |= PCI_CMD_BME | PCI_CMD_MSE |
		PCI_CMD_IOSE;

	// printk("AHCI Base Address: %x, cmd: %x\n", cfg->BAR5, cfg->hdr.cmd_status);
	map_mmio(k_pgtbl, KMMIO | cfg->BAR5, cfg->BAR5, NULL);
	lcr3(rcr3());
	sata_regs = (u32 *)(KMMIO | cfg->BAR5);

	// u16 cmd = pcie_readw(cfg, COMMAND);
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
	// int nports = (sata_regs[SATA_CAP] & CAP_NP) + 1;
	int ports_mask = sata_regs[SATA_PI];
	// 3. make sure all ports are in idle state
	// printk("IS.IPS: %x", sata_regs[SATA_IS]);
	ports = (struct sata_port_regs *)(sata_regs + (0x100 / 4));
	for (int i = 0; i < 32; ++i) {
		if (!(ports_mask & (1u << i)))
			continue;
		// port enabled
		u32 cmd = ports[i].cmd;
		if (!check_port(i))
			continue;
		printk("SATA port %d functional.\n", i);
		disk[n_disk].disk_ind = i;
		disk[n_disk++].driver_type = DISK_SATA;
		
		// printk("port %d cmd: %x\n", i, cmd);
		if (cmd & (CMD_ST | CMD_CR | CMD_FRE | CMD_FR)) {
			// 1) place into idle state with patience
			ports[i].cmd &= ~CMD_ST;
			if (ports[i].cmd & CMD_FRE) {
				ports[i].cmd &= ~CMD_FRE;
				while (ports[i].cmd & CMD_FR)
					;
			}
			int j;
			for (j = 0; j < 5000 && ports[i].cmd & (CMD_CR |CMD_FR); ++j)
				; // wait until CR cleared
			// printk("port %d cmd after reset: %x, j: %d\n", i, ports[i].cmd, j);
			// 2) port reset
			if (j == 5000) {
				// printk("port %d reset.\n", i);
				u32 sctl = ports[i].sctl;
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
		struct page_info *pg;
		pg = alloc_page(FLAG_ZERO);
		u64 paddr = pg->paddr;
		map_mmio(k_pgtbl, KMMIO | paddr, paddr, NULL);
		lcr3(rcr3());
		// cmd_list[i] = (struct sata_cmd_hdr *)(((u64)ports[i].clbu << 32) | ports[i].clb);
		cmd_list[i] = (struct sata_cmd_hdr *)(KMMIO | paddr);
		// printk("old cl %lx, old fb %lx\n", ((u64)ports[i].clbu << 32) | ports[i].clb,
		//     ((u64)ports[i].fbu << 32) | ports[i].fb);
		ports[i].clb = (u32)paddr; // 1K
		ports[i].clbu = paddr >> 32;
		paddr += (1 << 10);
		ports[i].fb = (u32)paddr; // 256 B
		ports[i].fbu = paddr >> 32;
	
		// 6. clear errors
		ports[i].serr = 0xFFFFFFFF;
		ports[i].is = 0;
		ports[i].ie = 0;

		// alloc memory for cmd_tbl
		// printk("AHCI port functional: %d, err: %x\n", i, ports[i].serr);

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
	// int ncs = ((sata_regs[SATA_CAP] & CAP_NCS) >> 8) + 1;

	// printk("AHCI nports: %d, ports implemented: %x, ncs: %d\n", nports, ports_mask, ncs);
}

int
sata_read_block(int did, u64 block)
{
	// AHCI Spec 1.3.1 Section 5.5
	int port = disk[did].disk_ind;
	int slot = 0;
	ports[port].is = 0xFFFF;
	// printk("port is: %x\n", ports[port].is);
	for (slot = 0; slot < 32; ++slot) {
		if ((ports[port].ci & (1u << slot)) || (ports[port].sact & (1u << slot)))
			continue;
		break;
	}
	if (slot == 32) {
		panic("no slot.\n");
		return -EBUSY;
	}
	struct sata_cmd_hdr *cmd_hdr = &cmd_list[port][slot];
	int prdtl = 1, cfl = sizeof(struct reg_h2d_fis) / 4, isatapi = 0;
	// build ATA command
	struct page_info *pg;
	static struct sata_cmd_tbl *cmd_tbl = NULL;
	// this function always use _the_ page for cmd_tbl
	if (cmd_tbl == NULL) {
		pg = alloc_page(FLAG_ZERO);
		cmd_tbl = (struct sata_cmd_tbl *)P2K(pg->paddr);
	}
	cmd_hdr->dw2 = (u32)(u64)cmd_tbl;
	cmd_hdr->dw3 = K2P(cmd_tbl) >> 32;

	// pte_t *ppte;
	// walk_pgtbl(k_pgtbl, cmd_tbl, &ppte, 0);
	// printk("cmd_tbl: %p, pte %lx\n", cmd_tbl, *ppte);
	// cmd_tbl->cfis
	struct reg_h2d_fis *cfis = &cmd_tbl->cfis.reg_h2d;
	cfis->fis_type = FIS_TYPE_REG_H2D;
	cfis->c = 1;
	cfis->command = ATA_COMMAND_READ_DMA_EXT;

	u64 lba = block * 8;
	cfis->lba0 = (u8)lba;
	cfis->lba1 = (u8)(lba >> 8);
	cfis->lba2 = (u8)(lba >> 16);
	cfis->device = 1<<6;

	cfis->lba3 = (u8)(lba >> 24);
	cfis->lba4 = (u8)(lba >> 32);
	cfis->lba5 = (u8)(lba >> 40);

	cfis->countl = 8; // 8 sector, 1 4K-block
	cfis->counth = 0;
	// cmd_tbl->acmd
	// cmd_tbl->prdt
	for (int i = 0; i < prdtl; ++i) {
		u64 vaddr = blk2kaddr(did, block);
		pte_t *pte;
		walk_pgtbl(k_pgtbl, vaddr, &pte, 1);
		if (*pte == 0) {
			pg = alloc_page(FLAG_ZERO);
			*pte = pg->paddr | PTE_P;
		}
		u64 paddr = PAGEADDR(*pte);
		// printk("read disk vaddr %lx, paddr: %lx\n", vaddr, paddr);
		cmd_tbl->prdt[i].dba = (u32)paddr;
		cmd_tbl->prdt[i].dbau = paddr >> 32;
		cmd_tbl->prdt[i].dw3 = (PGSIZE - 1);
	}

	// set cmd_hdr
	u32 dw0 = (prdtl << CMDHDR_PRDTL_SHIFT) | cfl;
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
	// printk("sata_read(): port: %d, slot: %d, prdbc: %d\n", port, slot, cmd_hdr[slot].dw1);
	return 0;
}

int
sata_write_block()
{
	// AHCI Spec 1.3.1 Section 5.5
	// TODO
	return 0;
}