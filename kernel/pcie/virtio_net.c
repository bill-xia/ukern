#include "pcie/virtio_net.h"

#include "pcie/pcie.h"

void
pcie_virtio_net_init(struct pcie_dev *dev, volatile struct pci_config_device *cfg)
{
	if ((cfg->hdr.status & PCI_STATUS_CAP) == 0) {
		printk("[WARN] No capability pointer.\n");
		return;
	}
	printk("status: %x\n", cfg->hdr.status);
	printk("capability_ptr: %x(%x & 0xFC)\n", (int)(cfg->cap_ptr & 0xFC), (int)(cfg->cap_ptr));
	volatile struct virtio_pci_cap *cap = (struct virtio_pci_cap *)
		((u8 *)cfg + (cfg->cap_ptr & 0xFC));
	while (1) {
		printk("- cfg_type: %d, bar%d[%d:%d]\n",
			(int)cap->cfg_type,
			(int)cap->bar,
			cap->offset,
			cap->offset + cap->length);
		if (cap->cap_next == 0) {
			break;
		} else {
			cap = (struct virtio_pci_cap *)
				((u8 *)cfg + (cap->cap_next & 0xFC));
		}
	}
}