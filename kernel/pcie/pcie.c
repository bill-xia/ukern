#include "pcie/pcie.h"
#include "pcie/nvme.h"
#include "pcie/sata.h"
#include "printk.h"

u64 pcie_base;
int n_pcie_dev_type, n_pcie_dev;
struct pcie_dev_type pcie_dev_type_list[NPCIEDEV];
struct pcie_dev pcie_dev_list[NPCIEDEV];

void scan_devfn(int bus, int devfn);
void pcie_init_device(int bus, int devfn);

void
scan_bus(int bus)
{
	// static vis = 0;
	// if (bus == 0 && vis) return;
	// if (bus == 0) vis = 1;
	// printk("scanning bus: %d\n", bus);
	int devfn = 0;
	for (devfn = 0; devfn < 0x100; devfn ++) {
		scan_devfn(bus, devfn);
	}
}

void
scan_devfn(int bus, int devfn)
{
	struct pci_config_hdr *hdr = (struct pci_config_hdr *)(pcie_base | (bus << 20) | (devfn << 12));
	if (pcie_readw(hdr, VENDORID) == 0xFFFF) // no device here
		return;
	if ((pcie_readb(hdr, HDRTYPE) & 0x7F) == 0x00) {
		pcie_init_device(bus, devfn);
	} else {
		struct pci_config_bridge *bridge = (struct pci_config_bridge *)hdr;
		// printk("bus found: bus %d, devfn %d, pri %d, sec %d, subord %d.\n",
		//     bus,
		//     devfn,
		//     (int)pcie_readb(bridge, PRIBUS),
		//     (int)pcie_readb(bridge, SECBUS),
		//     (int)pcie_readb(bridge, SUBBUS)
		// );
		scan_bus(pcie_readb(bridge, SECBUS));
	}
}

void
pcie_init_device(int bus, int devfn)
{
	struct pci_config_hdr *hdr = (struct pci_config_hdr *)(pcie_base | (bus << 20) | (devfn << 12));
	struct pci_config_device *dev = (struct pci_config_device *)hdr;
	// printk("dev [%d:%d]: vendor %x, dev_id %x, class %x, subclass %x, progif %x.\n",
	//     bus,
	//     devfn,
	//     (int)pcie_readw(dev, VENDORID),
	//     (int)pcie_readw(dev, DEVICEID),
	//     (int)pcie_readb(dev, CLASS),
	//     (int)pcie_readb(dev, SUBCLASS),
	//     (int)pcie_readb(dev, PROGIF)
	// );
	// u8	devid = pcie_readw(dev, DEVICEID),
	// 	vendor = pcie_readw(dev, VENDORID);
	u16	class = pcie_readb(dev, CLASS),
		subclass = pcie_readb(dev, SUBCLASS),
		progif = pcie_readb(dev, PROGIF);
	for (int i = 0; i < n_pcie_dev_type; ++i) {
		if (pcie_dev_type_list[i].class == class &&
			pcie_dev_type_list[i].subclass == subclass &&
			pcie_dev_type_list[i].progif == progif) { // match
			pcie_dev_type_list[i].dev_init(dev);
		}
	}
}

void
init_pcie(void)
{
	n_pcie_dev = 0;
	n_pcie_dev_type = 0;
	// register known devices
	pcie_nvme_register();
	pcie_sata_register();
	// scan available devices
	scan_bus(0);
}