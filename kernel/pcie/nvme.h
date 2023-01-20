#ifndef NVME_H
#define NVME_H

void pcie_nvme_register(void);
void pcie_nvme_io_init(struct pci_config_device *cfg);

#endif