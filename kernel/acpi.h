#ifndef ACPI_H
#define ACPI_H

#include "types.h"

struct RSDPDescriptor {
	char	Signature[8];
	u8	Checksum;
	char	OEMID[6];
	u8	Revision;
	u32	RsdtAddress;
} __attribute__ ((packed));

struct RSDPDescriptor20 {
	struct RSDPDescriptor	firstPart;
	u32	Length;
	u64	XsdtAddress;
	u8	ExtendedChecksum;
	u8	reserved[3];
} __attribute__ ((packed));

struct DescHeader {
	char	Signature[4];
	u32	Length;
	u8	Revision;
	u8	Checksum;
	char	OEMID[6];
	u64	OEMTableID;
	u32	OEMRevision;
	u32	CreatorID;
	u32	CreatorRevision;
} __attribute__ ((packed));

struct MCFG_entry {
	u64	ECAMBase;
	u16	PCISegGroupNum;
	u8	StartPCIBusNum,
		EndPCIBusNum,
		rsv[4];
} __attribute__ ((packed));

struct MADT_entry_hdr {
	u8	type,
		length;
} __attribute__ ((packed));

#define MADT_LAPIC  0x0
#define MADT_IOAPIC 0x1
#define MADT_ISO    0x2
#define MADT_NMI    0x3
#define MADT_LAPICNMI       0x4
#define MADT_LAPICAO        0x5
#define MADT_IOSAPIC        0x6
#define MADT_LSAPIC         0x7
#define MADT_PIS            0x8
#define MADT_Lx2APIC        0x9
#define MADT_Lx2APICNMI     0xA
#define MADT_GICC   0xB
#define MADT_GICD   0xC
#define MADT_MSI    0xD
#define MADT_GICR   0xE
#define MADT_ITS    0xF

struct madt_lapic {
	u8	type,
		length,
		puid,
		lapicid;
	u32	flags;
} __attribute__((packed));

struct madt_ioapic {
	u8	type,
		length,
		ioapicid,
		rsv;
	u32	addr,
		GSI_base;
} __attribute__((packed));

struct madt_iso {
	u8	type,
		length,
		bus,
		source;
	u32	GSI;
	u16	flags;
} __attribute__((packed));

int init_acpi();

#endif