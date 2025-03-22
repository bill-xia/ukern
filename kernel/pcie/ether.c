#include "ether.h"

#include "pcie/pcie.h"
#include "pcie/virtio_net.h"

void pcie_ethernet_init(volatile struct pci_config_device *cfg);

void
pcie_ethernet_register(void)
{
	struct pcie_dev_type *dev_type = &pcie_dev_type_list[n_pcie_dev_type++];
	dev_type->class = 0x02;
	dev_type->subclass = 0x00;
	dev_type->progif = 0x00;
	dev_type->dev_init = pcie_ethernet_init;
}

void
pcie_ethernet_init(volatile struct pci_config_device *cfg)
{
	struct pcie_dev *dev = &pcie_dev_list[n_pcie_dev++];
	dev->class = 0x02;
	dev->subclass = 0x00;
	dev->progif = 0x00;
	dev->cfg = cfg;
	dev->vendor = pcie_readw(cfg, VENDORID);
	dev->devid = pcie_readw(cfg, DEVICEID);
	printk("Found ethernet controller.\n");

	if (dev->vendor == VENDOR_VIRTIO_NET) {
		pcie_virtio_net_init(dev, cfg);
	} else {
		printk("Unknown vendor: %x, skipped.\n", dev->vendor);
	}
}