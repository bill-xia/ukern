#include "acpi.h"
#include "mem.h"
#include "kbd.h"
#include "printk.h"
#include "pcie/pcie.h"

void init_rsdp2(struct RSDPDescriptor20 *rsdp);
void init_rsdp(struct RSDPDescriptor *rsdp);
void acpi_init_mcfg(struct DescHeader *tbl);
void acpi_init_apic(struct DescHeader *tbl);

struct RSDPDescriptor*
detect_rsdp()
{
	u16 seg_ptr = *(u16 *)0x40E;
	// int r = 'R', s = 'S', d = 'D', p = 'P', t = 'T', r = 'R', sp = ' ';
	u64 *EBDA = (u64 *)P2K(seg_ptr << 4);
	for (int i = 0; i < 1024 / sizeof(u64); ++i) {
		if (*EBDA == 0x2052545020445352LL) {
			return (struct RSDPDescriptor*)EBDA;
		} else {
			EBDA++;
		}
	}
	EBDA = (u64 *)P2K(0x000E0000);
	while (EBDA < (u64 *)P2K(0x100000)) {
		if (*EBDA == 0x2052545020445352LL) {
			return (struct RSDPDescriptor*)EBDA;
		} else {
			EBDA++;
		}
	}
	return NULL;
}

int
init_acpi(struct RSDPDescriptor *rsdp)
{
	// struct RSDPDescriptor *rsdp = detect_rsdp();
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
	// printk("init_acpi done.\n");
	return 0;
}

void
init_rsdp2(struct RSDPDescriptor20 *rsdp)
{
	printk("RSDP 2.0\n");
	u8 chksum = 0, *cursor = (u8 *)rsdp;
	for (int i = 0; i < sizeof(struct RSDPDescriptor20); ++i) {
		chksum += cursor[i];
	}
	if (chksum != 0) {
		panic("checksum neq 0\n");
	}
	// printk("rsdp descriptor: %p\n", rsdp);
	// printk("rsdp revision: %d\n", rsdp->firstPart.Revision);
	// printk("rsdp addr: %p\n", rsdp->XsdtAddress);
	struct DescHeader *xsdt_desc = (struct DescHeader *)P2K(rsdp->XsdtAddress);
	int n_xsdt_entries = (xsdt_desc->Length - sizeof(struct DescHeader)) / 8;
	u64 *xsdt = (u64 *)P2K(xsdt_desc + 1);
	// printk("# of xsdt entries: %d\n", n_xsdt_entries);
	for (int i = 0; i < n_xsdt_entries; ++i) {
		struct DescHeader *tbl = (struct DescHeader *)P2K(xsdt[i]);
		u32 sign = 0;
		for (int j = 3; j >= 0; --j) {
			sign = (sign << 8) + tbl->Signature[j];
		}
		// printk("signature: %c%c%c%c\n", sign>>24, (sign>>16)&255, (sign>>8)&255, sign & 255);
		switch (sign) {
		case 0x4746434d: ; // MCFG
			acpi_init_mcfg(tbl);
			break;
		case 0x43495041: ; // APIC
			acpi_init_apic(tbl);
			break;
		default:
			break;
		}
	}
}

void
init_rsdp(struct RSDPDescriptor *rsdp)
{
	printk("RSDP 1.0\n");
	u8 chksum = 0, *cursor = (u8 *)rsdp;
	for (int i = 0; i < sizeof(struct RSDPDescriptor); ++i) {
		chksum += cursor[i];
	}
	if (chksum != 0) {
		panic("RSDP hecksum neq 0\n");
	}
	// printk("rsdp descriptor: %p\n", rsdp);
	// printk("rsdp revision: %d\n", rsdp->Revision);
	// printk("rsdp addr: %p\n", rsdp->RsdtAddress);
	struct DescHeader *rsdt_desc = (struct DescHeader *)P2K(rsdp->RsdtAddress);
	int n_rsdt_entries = (rsdt_desc->Length - sizeof(struct DescHeader)) / 4;
	u32 *rsdt = (u32 *)P2K(rsdt_desc + 1);
	// printk("# of rsdt entries: %d\n", n_rsdt_entries);
	for (int i = 0; i < n_rsdt_entries; ++i) {
		struct DescHeader *tbl = (struct DescHeader *)P2K(rsdt[i]);
		u32 sign = 0;
		for (int j = 3; j >= 0; --j) {
			sign = (sign << 8) + tbl->Signature[j];
		}
		// printk("signature: %c%c%c%c\n", sign>>24, (sign>>16)&255, (sign>>8)&255, sign & 255);
		switch (sign) {
		case 0x4746434d: ; // MCFG
			acpi_init_mcfg(tbl);
			break;
		case 0x43495041: ; // APIC
			acpi_init_apic(tbl);
			break;
		default:
			break;
		}
	}
}

void
acpi_init_mcfg(struct DescHeader *tbl)
{
	printk("ACPI: Found MCFG entry. Initializing PCIe.\n");
	struct MCFG_entry *mcfg_ent = (struct MCFG_entry *)((u64)tbl + sizeof(struct DescHeader) + 8);
	// printk("tbl->Length: %d\n", tbl->Length);
	int n_mcfg_ent = (tbl->Length - sizeof(struct DescHeader) - 8) / sizeof(struct MCFG_entry), group = 1 << 16;
	// printk("n_mcfg_ent: %d\n", n_mcfg_ent);
	pte_t *pte;
	for (int i = 0; i < n_mcfg_ent; ++i) {
		if (group == (1 << 16)) {
			group = mcfg_ent[i].PCISegGroupNum;
		}
		if (mcfg_ent[i].PCISegGroupNum != group) {
			panic("MCFG: multiple PCIe Segment groups exists!\n");
		}
		// printk("group: %d, busnum: %x\n", group, busnum);
		// printk("ECAMbase: %p\n", mcfg_ent[i].ECAMBase);
		// if ((mcfg_ent[i].ECAMBase & (255 << 20)) != ((u32)mcfg_ent[i].StartPCIBusNum << 20)) {
		//     panic("MCFG: ECAMBase not aligned by bus num.\n");
		// }
		pcie_base = KMMIO | mcfg_ent[i].ECAMBase;
		// Setup PCIe ECAM MMIO
		// printk("start bus: %d, end bus: %d, base: %x\n", mcfg_ent[i].StartPCIBusNum, mcfg_ent[i].EndPCIBusNum, mcfg_ent[i].ECAMBase);
		int bus;
		for (bus = 0; bus <= mcfg_ent[i].EndPCIBusNum - mcfg_ent[i].StartPCIBusNum; ++bus) {
			u64 base = mcfg_ent[i].ECAMBase + (bus << 20);
			// printk("mapping %p onto %p\n", base, base | KMMIO);
			for (int j = 0; j < 256 * PGSIZE; j += PGSIZE)
				map_mmio(k_pgtbl, KMMIO | (base + j), (base + j), &pte);
		}
	}
	init_pcie();
}

void
acpi_init_apic(struct DescHeader *tbl)
{
	printk("ACPI: Found MADT entry. Initializing IOAPIC.\n");
	struct MADT_entry_hdr *madt_ent_hdr;
	int ps2kbd_valid = (*(u32*)((u64)tbl + sizeof(struct DescHeader) + 4)) & 0x1;
	if (ps2kbd_valid) {
		printk("Found PS/2 Keyboard.\n");
		init_8042();
	}
	u8 *ptr = (u8 *)((u64)tbl + sizeof(struct DescHeader) + 8),
			*end = (u8 *)tbl + tbl->Length;
	while (ptr < end) {
		madt_ent_hdr = (struct MADT_entry_hdr *)ptr;
		
		switch (madt_ent_hdr->type) {
		case MADT_LAPIC: ;
			struct madt_lapic *lapic = (struct madt_lapic *)ptr;
			printk("LAPIC: puid %d, lapicid %d, flags %x\n",
			    lapic->puid, lapic->lapicid, lapic->flags);
			break;
		case MADT_IOAPIC: ;
			struct madt_ioapic *ioapic = (struct madt_ioapic *)ptr;
			printk("IOAPIC: ioapicid %x, addr %x, intr_base %x\n",
			    (u32)ioapic->ioapicid, ioapic->addr, ioapic->GSI_base);
			break;
		case MADT_ISO: ;
			struct madt_iso *iso = (struct madt_iso *)ptr;
			printk("ISO: source %d, GSI %d, flags %x\n",
			    (u32)iso->source, iso->GSI, (u32)iso->flags);
			break;
		case MADT_NMI: ;
			struct madt_nmi *nmi = (struct madt_nmi *)ptr;
			printk("NMI: flags %x, GSI %d\n",
				(u32)nmi->flags,
				nmi->GSI);
			break;
		case MADT_LAPICNMI: ;
			struct madt_lapicnmi *lnmi = (struct madt_lapicnmi *)ptr;
			printk("LAPICNMI: PUID %x, flags %x, LINT %x\n",
				(u32)lnmi->puid,
				lnmi->flags,
				lnmi->LINT);
			break;
		case MADT_LAPICAO: ;
			struct madt_lapicao *lao = (struct madt_lapicao *)ptr;
			printk("LAPICAO: LAPIC addr %lx\n",
				lao->paddr);
			break;
		case MADT_IOSAPIC: ;
			struct madt_iosapic *iosapic = (struct madt_iosapic *)ptr;
			printk("IOAPIC: ioapicid %x, addr %x, intr_base %x\n",
			    (u32)iosapic->ioapicid,
			    iosapic->paddr,
			    iosapic->GSI_base);
			break;
		case MADT_LSAPIC: ;
			struct madt_lsapic *lsapic = (struct madt_lsapic *)ptr;
			printk("LSAPIC: pid %d, lid %d, leid %d, flags %x, puid %d, puid_str %s",
				(u32)lsapic->pid,
				(u32)lsapic->lsapicid,
				(u32)lsapic->lsapiceid,
				lsapic->flags,
				lsapic->puid,
				lsapic->puid_str);
			break;
		case MADT_PIS: ;
			struct madt_pis *pis = (struct madt_pis *)ptr;
			printk("PIS: flags %x, intr_type %d, pid %d, peid %d, iosapic_vec %d,"
				"GSI %d, PIS_Flags %x\n",
				pis->flags,
				pis->intr_type,
				pis->pid,
				pis->peid,
				pis->iosapic_vec,
				pis->GSI,
				pis->pis_flags);
			break;
		case MADT_Lx2APIC: ;
			struct madt_lx2apic *lx2apic = (struct madt_lx2apic *)ptr;
			printk("Lx2APIC: lapicid %d, flags %x, puid %d\n",
				lx2apic->x2apic_id,
				lx2apic->flags,
				lx2apic->puid);
			break;
		case MADT_Lx2APICNMI:
			struct madt_lx2apicnmi *lx2apicnmi = (struct madt_lx2apicnmi *)ptr;
			printk("Lx2APICNMI: flags %x, puid %d, LINT %d\n",
				lx2apicnmi->flags,
				lx2apicnmi->puid,
				lx2apicnmi->LINT);
			break;
		default:
			break;
		}
		ptr += madt_ent_hdr->length;
	}
}