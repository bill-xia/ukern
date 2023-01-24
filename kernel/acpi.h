#ifndef ACPI_H
#define ACPI_H

#include "types.h"

struct RSDPDescriptor {
    char Signature[8];
    uint8_t Checksum;
    char OEMID[6];
    uint8_t Revision;
    uint32_t RsdtAddress;
} __attribute__ ((packed));

struct RSDPDescriptor20 {
    struct RSDPDescriptor firstPart;
    uint32_t Length;
    uint64_t XsdtAddress;
    uint8_t ExtendedChecksum;
    uint8_t reserved[3];
} __attribute__ ((packed));

struct DescHeader {
    char Signature[4];
    uint32_t Length;
    uint8_t Revision;
    uint8_t Checksum;
    char OEMID[6];
    uint64_t OEMTableID;
    uint32_t OEMRevision;
    uint32_t CreatorID;
    uint32_t CreatorRevision;
} __attribute__ ((packed));

struct MCFG_entry {
    uint64_t ECAMBase;
    uint16_t PCISegGroupNum;
    uint8_t  StartPCIBusNum, EndPCIBusNum, rsv[4];
} __attribute__ ((packed));

struct MADT_entry_hdr {
    uint8_t type,
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
    uint8_t type,
            length,
            puid,
            lapicid;
    uint32_t    flags;
} __attribute__((packed));

struct madt_ioapic {
    uint8_t type,
            length,
            ioapicid,
            rsv;
    uint32_t    addr,
                GSI_base;
} __attribute__((packed));

struct madt_iso {
    uint8_t type,
            length,
            bus,
            source;
    uint32_t    GSI;
    uint16_t    flags;
} __attribute__((packed));

int init_acpi();

#endif