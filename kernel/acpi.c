#include "acpi.h"
#include "mem.h"

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
    if (rsdp != NULL) {
        uint8_t chksum = 0, *cursor = rsdp;
        for (int i = 0; i < 20; ++i) {
            chksum += cursor[i];
        }
        if (chksum != 0) {
            printk("checksum neq 0\n");
        }
        printk("rsdp descriptor: %p\n", rsdp);
        printk("rsdp revision: %d\n", rsdp->Revision);
        printk("rsdp addr: %p\n", rsdp->RsdtAddress);
        struct DescHeader *rsdt = P2K(rsdp->RsdtAddress);
        int n_rsdt_entries = (rsdt->Length - sizeof(struct DescHeader)) / 4;
        uint32_t tbl_ptr = (uint32_t)rsdt + sizeof(struct DescHeader);
        printk("# of rsdt entries: %d\n", n_rsdt_entries);
        printk("tbl_ptr: %p\n", tbl_ptr);
        for (int i = 0; i < n_rsdt_entries; ++i, tbl_ptr += 4) {
            struct DescHeader *tbl = (struct DescHeader *)*(uint32_t *)(P2K(tbl_ptr));
            // printk("tbl: %p\n", tbl);
            // printk("signature: %s\n", tbl->Signature);
            uint32_t sign = 0;
            for (int j = 3; j >= 0; --j) {
                sign = (sign << 8) + tbl->Signature[j];
            }
            switch (sign) {
            case 0x4746434d: ; // MCFG
                printk("found mcfg entry\n");
                struct MCFG_entry *mcfg_ent = (struct MCFG_entry *)(tbl + 44);
                int n_mcfg_ent = (tbl->Length - 44) / 16;
                printk("n_mcfg_ent: %d", n_mcfg_ent);
                break;
            default:
                // printk("signature: %x\n", sign);
                break;
            }
        }
    } else {
        printk("rsdp is NULL\n");
    }
}