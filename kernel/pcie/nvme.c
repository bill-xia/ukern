#include "pcie/pcie.h"
#include "pcie/nvme.h"
#include "printk.h"

void
pcie_nvme_register(void)
{
	struct pcie_dev_type *dev_type = &pcie_dev_type_list[n_pcie_dev_type++];
	dev_type->class = 0x01;
	dev_type->subclass = 0x08;
	dev_type->progif = 0x02;
	dev_type->dev_init = pcie_nvme_io_init;
}

void
pcie_nvme_io_init(volatile struct pci_config_device *cfg)
{
	struct pcie_dev *dev = &pcie_dev_list[n_pcie_dev++];
	dev->class = 0x01;
	dev->subclass = 0x08;
	dev->progif = 0x02;
	dev->cfg = cfg;
	dev->vendor = pcie_readw(cfg, VENDORID);
	dev->devid = pcie_readw(cfg, DEVICEID);
	printk("found nvme controller.\n");
}