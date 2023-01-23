#include "acpi.h"
#include "mem.h"
#include "printk.h"
#include "pcie/pcie.h"

void init_rsdp2(struct RSDPDescriptor20 *rsdp);
void init_rsdp(struct RSDPDescriptor *rsdp);
void acpi_init_mcfg(struct DescHeader *tbl);

struct RSDPDescriptor*
detect_rsdp()
{
    uint16_t seg_ptr = *(uint16_t *)0x40E;
    // int r = 'R', s = 'S', d = 'D', p = 'P', t = 'T', r = 'R', sp = ' ';
    uint64_t *EBDA = (uint64_t *)P2K(seg_ptr << 4);
    for (int i = 0; i < 1024 / sizeof(uint64_t); ++i) {
        if (*EBDA == 0x2052545020445352LL) {
            return (struct RSDPDescriptor*)EBDA;
        } else {
            EBDA++;
        }
    }
    EBDA = (uint64_t *)P2K(0x000E0000);
    while (EBDA < (uint64_t *)P2K(0x100000)) {
        if (*EBDA == 0x2052545020445352LL) {
            return (struct RSDPDescriptor*)EBDA;
        } else {
            EBDA++;
        }
    }
    return NULL;
}

int
init_acpi()
{
    struct RSDPDescriptor *rsdp = detect_rsdp();
    if (rsdp == NULL) {
        panic("rsdp is NULL\n");
    }
    if (rsdp->Revision == 2) {
        // XSDP
        init_rsdp2((struct RSDPDescriptor20 *)rsdp);
    } else if (rsdp->Revision == 0) {
        // RSDP
        init_rsdp(rsdp);
    } else {
        panic("RSDP revision unknown.\n");
    }
    return 0;
}

void
init_rsdp2(struct RSDPDescriptor20 *rsdp)
{
    int i, j;

    printk("RSDP 2.0\n");
    uint8_t chksum = 0, *cursor = (uint8_t *)rsdp;
    for (i = 0; i < sizeof(struct RSDPDescriptor20); ++i) {
        chksum += cursor[i];
    }
    if (chksum != 0) {
        panic("checksum neq 0\n");
    }
    printk("rsdp descriptor: %p\n", rsdp);
    printk("rsdp revision: %d\n", rsdp->firstPart.Revision);
    printk("rsdp addr: %p\n", rsdp->XsdtAddress);
    struct DescHeader *xsdt_desc = (struct DescHeader *)P2K(rsdp->XsdtAddress);
    int n_xsdt_entries = (xsdt_desc->Length - sizeof(struct DescHeader)) / 8;
    uint64_t *xsdt = (uint64_t *)P2K(xsdt_desc + 1);
    printk("# of xsdt entries: %d\n", n_xsdt_entries);
    for (i = 0; i < n_xsdt_entries; ++i) {
        struct DescHeader *tbl = (struct DescHeader *)P2K(xsdt[i]);
        // printk("tbl: %p\n", tbl);
        // printk("signature: %s\n", tbl->Signature);
        uint32_t sign = 0;
        for (j = 3; j >= 0; --j) {
            sign = (sign << 8) + tbl->Signature[j];
        }
        switch (sign) {
        case 0x4746434d: ; // MCFG
            acpi_init_mcfg(tbl);
            break;
        default:
            // printk("signature: %c%c%c%c\n", sign>>24, (sign>>16)&255, (sign>>8)&255, sign & 255);
            break;
        }
    }
}

void
init_rsdp(struct RSDPDescriptor *rsdp)
{
    int i, j;

    printk("RSDP 1.0\n");
    uint8_t chksum = 0, *cursor = (uint8_t *)rsdp;
    for (i = 0; i < sizeof(struct RSDPDescriptor); ++i) {
        chksum += cursor[i];
    }
    if (chksum != 0) {
        panic("RSDP hecksum neq 0\n");
    }
    printk("rsdp descriptor: %p\n", rsdp);
    printk("rsdp revision: %d\n", rsdp->Revision);
    printk("rsdp addr: %p\n", rsdp->RsdtAddress);
    struct DescHeader *rsdt_desc = (struct DescHeader *)P2K(rsdp->RsdtAddress);
    int n_rsdt_entries = (rsdt_desc->Length - sizeof(struct DescHeader)) / 4;
    uint32_t *rsdt = (uint32_t *)P2K(rsdt_desc + 1);
    printk("# of rsdt entries: %d\n", n_rsdt_entries);
    for (i = 0; i < n_rsdt_entries; ++i) {
        struct DescHeader *tbl = (struct DescHeader *)P2K(rsdt[i]);
        // printk("tbl: %p\n", tbl);
        // printk("signature: %s\n", tbl->Signature);
        uint32_t sign = 0;
        for (j = 3; j >= 0; --j) {
            sign = (sign << 8) + tbl->Signature[j];
        }
        switch (sign) {
        case 0x4746434d: ; // MCFG
            acpi_init_mcfg(tbl);
            break;
        default:
            // printk("signature: %c%c%c%c\n", sign>>24, (sign>>16)&255, (sign>>8)&255, sign & 255);
            break;
        }
    }
}

void
acpi_init_mcfg(struct DescHeader *tbl)
{
    int i, j;
    printk("found mcfg entry\n");
    struct MCFG_entry *mcfg_ent = (struct MCFG_entry *)((uint64_t)tbl + sizeof(struct DescHeader) + 8);
    printk("tbl->Length: %d\n", tbl->Length);
    int n_mcfg_ent = (tbl->Length - sizeof(struct DescHeader) - 8) / sizeof(struct MCFG_entry), group = 1 << 16;
    printk("n_mcfg_ent: %d\n", n_mcfg_ent);
    pte_t *pte;
    for (i = 0; i < n_mcfg_ent; ++i) {
        if (group == (1 << 16)) {
            group = mcfg_ent[i].PCISegGroupNum;
        }
        if (mcfg_ent[i].PCISegGroupNum != group) {
            panic("MCFG: multiple PCIe Segment groups exists!\n");
        }
        // printk("group: %d, busnum: %x\n", group, busnum);
        // printk("ECAMbase: %p\n", mcfg_ent[i].ECAMBase);
        // if ((mcfg_ent[i].ECAMBase & (255 << 20)) != ((uint32_t)mcfg_ent[i].StartPCIBusNum << 20)) {
        //     panic("MCFG: ECAMBase not aligned by bus num.\n");
        // }
        pcie_base = KMMIO | mcfg_ent[i].ECAMBase;
        // Setup PCIe ECAM MMIO
        printk("start bus: %d, end bus: %d, base: %x\n", mcfg_ent[i].StartPCIBusNum, mcfg_ent[i].EndPCIBusNum, mcfg_ent[i].ECAMBase);
        int bus;
        for (bus = 0; bus <= mcfg_ent[i].EndPCIBusNum - mcfg_ent[i].StartPCIBusNum; ++bus) {
            uint64_t base = mcfg_ent[i].ECAMBase + (bus << 20);
            // printk("mapping %p onto %p\n", base, base | KMMIO);
            for (j = 0; j < 256 * PGSIZE; j += PGSIZE)
                map_mmio(k_pgtbl, KMMIO | (base + j), (base + j), &pte);
        }
    }
    init_pcie();
}