#ifndef VIRTIO_NET_H
#define VIRTIO_NET_H

#include "pcie/pcie.h"

struct virtio_pci_cap {
	u8 cap_vndr; /* Generic PCI field: PCI_CAP_ID_VNDR */
	u8 cap_next; /* Generic PCI field: next ptr. */
	u8 cap_len; /* Generic PCI field: capability length */

	u8 cfg_type; /* Identifies the structure. */
/* Common configuration */
#define VIRTIO_PCI_CAP_COMMON_CFG 1
/* Notifications */
#define VIRTIO_PCI_CAP_NOTIFY_CFG 2
/* ISR Status */
#define VIRTIO_PCI_CAP_ISR_CFG 3
/* Device specific configuration */
#define VIRTIO_PCI_CAP_DEVICE_CFG 4
/* PCI configuration access */
#define VIRTIO_PCI_CAP_PCI_CFG 5
/* Shared memory region */
#define VIRTIO_PCI_CAP_SHARED_MEMORY_CFG 8
/* Vendor-specific data */
#define VIRTIO_PCI_CAP_VENDOR_CFG 9

	u8 bar; /* Where to find it. */
	u8 id; /* Multiple capabilities of the same type */
	u8 padding[2]; /* Pad to full dword. */
	le32 offset; /* Offset within bar. */
	le32 length; /* Length of the structure, in bytes. */
};

void
pcie_virtio_net_init(struct pcie_dev *dev, volatile struct pci_config_device *cfg);

#endif