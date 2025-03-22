#ifndef ETHER_H
#define ETHER_H

#include "pcie/pcie.h"

#define VENDOR_VIRTIO_NET 0x1AF4

void pcie_ethernet_register(void);

#endif