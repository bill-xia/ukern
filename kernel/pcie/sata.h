#ifndef SATA_H
#define SATA_H

#include "mem.h"
#include "pcie/pcie.h"
#include "fs/fs.h"

extern uint32_t *sata_regs;

#define SATA_CAP (0x00 / 4)
#define SATA_GHC (0x04 / 4)
#define SATA_IS (0x08 / 4)
#define SATA_PI (0x0C / 4)
#define SATA_BOHC (0x28 / 4)
    #define BOHC_OOS    0x02
    #define BOHC_BOS    0x01

#define GHC_AE  0x80000000
#define GHC_IE  0x2
#define GHC_RST  0x1

#define SATA_IE_ALL 0x7CC0007F

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

#define TFD_BSY 0x80
#define TFD_DRQ 0x04
#define TFD_ERR 0x1

#define SSTS_DET    0x0F

#define SCTL_DET    0x0000000F

struct sata_port_regs {
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

#define CMDHDR_PRDTL_SHIFT 16
#define CMDHDR_A    0x20
#define CMDHDR_W    0x40
#define CMDHDR_P    0x80
#define CMDHDR_R    0x100
#define CMDHDR_BIST 0x200
#define CMDHDR_C    0x400

struct sata_cmd_hdr {
    uint32_t    dw0,
                dw1,
                dw2,
                dw3,
                dw4,
                dw5,
                dw6,
                dw7;
} __attribute__ ((packed));

struct CFIS {
    char cfis[64];
};

#define PRDT_I 0x80000000

struct sata_prdt {
    uint32_t    dba,
                dbau,
                rsv,
                dw3;
};

#define FIS_TYPE_REG_H2D    0x27

struct reg_h2d_fis {
	// DWORD 0
	uint8_t  fis_type;	// FIS_TYPE_REG_H2D
 
	uint8_t  pmport:4;	// Port multiplier
	uint8_t  rsv0:3;		// Reserved
	uint8_t  c:1;		// 1: Command, 0: Control
 
	uint8_t  command;	// Command register
	uint8_t  featurel;	// Feature register, 7:0
 
	// DWORD 1
	uint8_t  lba0;		// LBA low register, 7:0
	uint8_t  lba1;		// LBA mid register, 15:8
	uint8_t  lba2;		// LBA high register, 23:16
	uint8_t  device;		// Device register
 
	// DWORD 2
	uint8_t  lba3;		// LBA register, 31:24
	uint8_t  lba4;		// LBA register, 39:32
	uint8_t  lba5;		// LBA register, 47:40
	uint8_t  featureh;	// Feature register, 15:8
 
	// DWORD 3
	uint8_t  countl;		// Count register, 7:0
	uint8_t  counth;		// Count register, 15:8
	uint8_t  icc;		// Isochronous command completion
	uint8_t  control;	// Control register
 
	// DWORD 4
	uint8_t  rsv1[4];	// Reserved
};

struct reg_d2h_fis {
    //
};

struct set_dev_bits_fis {
    //
};

struct dma_activate_fis {
    //
};

struct dma_setup_fis {
    //
};

struct bist_activate_fis {
    //
};

struct pio_setup_fis {
    //
};

struct data_fis {
    //
};

struct sata_cmd_tbl {
    union {
        struct reg_h2d_fis reg_h2d;
        struct reg_d2h_fis reg_d2h;
        struct set_dev_bits_fis set_dev_bits;
        struct dma_activate_fis dma_activate;
        struct dma_setup_fis dma_setup;
        struct bist_activate_fis bist_activate;
        struct pio_setup_fis pio_setup;
        struct data_fis data;
        char pad[0x40];
    } cfis;
    uint8_t acmd[0x10];
    uint8_t rsv[0x30];
    struct sata_prdt prdt[0];
} __attribute__ ((packed));

void pcie_sata_register(void);
void pcie_sata_ahci_init(volatile struct pci_config_device *cfg);
int sata_read_block(int did, uint64_t block);

#endif