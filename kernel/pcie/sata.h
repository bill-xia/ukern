#ifndef SATA_H
#define SATA_H

extern volatile uint32_t *sata_regs;

#define SATA_CAP (0x00 / 4)
#define SATA_GHC (0x04 / 4)
#define SATA_PI (0x0C / 4)

#define GHC_AE  0x80000000

#define CAP_NP  0x0000001F
#define CAP_NCS 0x00001F00
#define CAP_S64A    0x80000000

#define CMD_ST  0x00000001
#define CMD_CR  0x00008000
#define CMD_FRE 0x00000010
#define CMD_FR  0x00004000

#define SERR_ALL    0x07FF0F03

#define STS_BSY 0x80
#define STS_DRQ 0x08
#define STS_ERR 0x01

#define SCTL_DET    0x0000000F

volatile struct sata_port_regs {
    uint32_t    clb,
                clbu,
                fb,
                fbu,
                is,
                ie,
                cmd,
                rsv,
                tfd,
                sig,
                ssts,
                sctl,
                serr,
                sact,
                ci,
                sntf,
                fbs,
                devslp,
                rsv1[10],
                vs[4];
} __attribute__ ((packed));

void pcie_sata_register(void);
void pcie_sata_ahci_init(struct pci_config_device *cfg);

#endif