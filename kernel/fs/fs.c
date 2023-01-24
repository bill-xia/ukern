#include "fs/fs.h"
#include "pcie/sata.h"
#include "fs/exfat.h"

int n_disk;
struct disk_t disk[32];
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
disk_read(int did, uint64_t lba, int n_sec)
{
    if (did < 0 || did >= n_disk)
        panic("disk_read_lba(): invalid did %d, n_disk %d.\n", did, n_disk);
    
    int i, r;
    switch (disk[did].driver_type) {
    case DISK_SATA:
        for (i = 0; i < (n_sec - 1) / 8 + 1; ++i) {
            if ((r = sata_read_block(did, lba / 8)) < 0)
                return r;
            lba += 8;
        }
        return 0;
    default:
        printk("disk_read_lba(): unknown disk driver type %d.\n", disk[did].driver_type);
        return -1;
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