#include "fs/fs.h"
#include "pcie/sata.h"
#include "fs/exfat.h"

struct FS_exFAT rtfs;

int
detect_fs(int did, int pid)
{
	disk_read(did, disk[did].part[pid].lba_beg, 8);
	// printk("lba_beg: %d\n", disk[did].part[pid].lba_beg);
	struct exFAT_hdr *exfat_hdr = (struct exFAT_hdr *)lba2kaddr(did, disk[did].part[pid].lba_beg);
	if (strcmp(exfat_hdr->fs_name, "EXFAT   ", 8) == 0) {
		rtfs.did = did;
		rtfs.part = &disk[did].part[pid];
		init_fs_exfat(&rtfs);
		return FS_EXFAT;
	} else {
		return FS_UNKNOWN;
	}
}

int
open_file(const char *filename, struct file_desc *fdesc)
{
	// assume _the_ exFAT temporarily
	return exfat_open_file(&rtfs, filename, fdesc);
}

int
read_file(char *dst, size_t sz, struct file_desc *fdesc)
{
	// assume _the_ exFAT temporarily
	return exfat_read_file(&rtfs, dst, sz, fdesc);
}