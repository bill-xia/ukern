#include "fs/fs.h"
#include "pcie/sata.h"
#include "fs/exfat.h"
#include "string.h"

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
open_file(const char *filename, struct file_desc *pwd, struct file_desc *fdesc)
{
	// assume _the_ exFAT temporarily
	return exfat_open(&rtfs, filename, pwd, fdesc, 0);
}

int
open_dir(const char *dirname, struct file_desc *pwd, struct file_desc *fdesc)
{
	// assume _the_ exFAT temporarily
	return exfat_open(&rtfs, dirname, pwd, fdesc, 1);
}

int
read_file(char *dst, size_t sz, struct file_desc *fdesc)
{
	// assume _the_ exFAT temporarily
	return exfat_read_file(&rtfs, dst, sz, fdesc);
}

int
read_dir(struct dirent *dst, struct file_desc *fdesc)
{
	// assume _the_ exFAT temporarily
	return exfat_read_dir(&rtfs, dst, fdesc);
}

int
path_push(char *dst, const char *src)
{
	int r = 1 + strlen(src);
	strcat(strcat(dst, "/"), src);
	return r;
}

int
path_pop(char *src, int len)
{
	// pop at root, ignore
	if (len == 0)
		return 0;
	assert(src[0] == '/');
	for (int i = len - 1; i >= 0; --i) {
		if (src[i] == '/') {
			src[i] = '\0';
			return len - i;
		}
	}
	panic("path_pop to root.\n");
	return -1;
}