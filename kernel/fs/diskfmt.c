#include "diskfmt.h"
#include "mem.h"
#include "pcie/sata.h"
#include "printk.h"
#include "fs/fs.h"

int n_disk;
struct disk_t disk[32];
struct guid known_guid[] = {
    {0, 0, 0, {0}}, // Empty

    {0x024DEE41, 0x33E7, 0x11D3, {0x9D, 0x69, 0x00, 0x08, 0xC7, 0x81, 0xF3, 0x9F}}, // MBR partition
    {0x21686148, 0x6449, 0x6E6F, {0x74, 0x4E, 0x65, 0x65, 0x64, 0x45, 0x46, 0x49}}, // BIOS boot partition
    {0xC12A7328, 0xF81F, 0x11D2, {0xBA, 0x4B, 0x00, 0xA0, 0xC9, 0x3E, 0xC9, 0x3B}}, // EFI

    {0xE3C9E316, 0x0B5C, 0x4DB8, {0x81, 0x7D, 0xF9, 0x2D, 0xF0, 0x02, 0x15, 0xAE}}, // MSR
    {0x5808C8AA, 0x7E8F, 0x42E0, {0x85, 0xD2, 0xE1, 0xE9, 0x04, 0x34, 0xCF, 0xB3}}, // LDM meta
    {0xAF9B60A0, 0x1431, 0x4F62, {0xBC, 0x68, 0x33, 0x11, 0x71, 0x4A, 0x69, 0xAD}}, // LDM data
    {0xDE94BBA4, 0x06D1, 0x4D40, {0xA1, 0x6A, 0xBF, 0xD5, 0x01, 0x79, 0xD6, 0xAC}}, // Windows Recovery
    {0xEBD0A0A2, 0xB9E5, 0x4433, {0x87, 0xC0, 0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7}}, // Windows Basic Data

    {0x0FC63DAF, 0x8483, 0x4772, {0x8E, 0x79, 0x3D, 0x69, 0xD8, 0x47, 0x7D, 0xE4}}, // Linux FS
};

const char *guid_name[] = {
    "Empty partition",
    "MBR partition scheme",
    "BIOS boot partition",
    "EFI partition",
    "MSR partition",
    "LDM meta partition",
    "LDM data partition",
    "Windows Recovery partition",
    "Windows Basic Data partition",
    "Linux FS partition"
};

enum guid_type {
    GUID_EMPTY,
    GUID_MBR,
    GUID_BIOS,
    GUID_EFI,
    GUID_MSR,
    GUID_LDM_META,
    GUID_LDM_DATA,
    GUID_WIN_RECOVERY,
    GUID_WIN_DATA,
    GUID_LINUX_FS,
    N_KNOWN_GUID
};

int init_mbr(int did);
int init_gpt(int did);

int
ls_diskpart()
{
    int i, did = 0, blk = 0;
    for (did = 0; did < n_disk; ++did) {
        init_mbr(did);
    }
}

int
init_gpt(int did)
{
    int i, blk, r;
    if (disk[did].driver_type == DISK_SATA) {
        for (blk = 1; blk <= 5; ++blk) {
            sata_read_block(did, blk);
        }
    } else {
        panic("unknown disk type.\n");
    }
    struct GPT *gpt = (struct GPT *)(blk2kaddr(did, 0) + 512);
    if (gpt->sign != 0x5452415020494645ul) {
        printk("GPT signature error.\n");
        return -1;
    }
    // printk("GPT sig: %lx, lbabeg: %ld, lbaend: %ld, parlba: %ld, n_par: %d, entrysiz: %d\n",
        // gpt->sign, gpt->lba_beg, gpt->lba_end, gpt->lba_pararr, gpt->n_pararr, gpt->par_entry_siz);
    char *ptr = (char *)gpt + 512;
    int n_part = gpt->n_pararr, par_entry_siz = gpt->par_entry_siz;
    printk("GPT partition number: %d\n", n_part);
    for (i = 0; i < n_part; ++i, ptr += par_entry_siz) {
        struct GPTPAR *par = (struct GPTPAR *)ptr;
        int j;
        for (j = 0; j < N_KNOWN_GUID; ++j) {
            if (guid_eq(&par->type_guid, &known_guid[j])) {
                if (j == 0)
                    break;
                int pid = disk[did].n_part++;
                disk[did].part[pid].part_type = PARTTYPE_GPT | j;
                disk[did].part[pid].lba_beg = par->lba_beg;
                disk[did].part[pid].n_sec = par->lba_end - par->lba_beg + 1;
                printk("GPT (sd%d,p%d) type: %s\n", did, i, guid_name[j]);
                if (j == GUID_WIN_DATA) {
                    r = detect_fs(did, disk[did].n_part - 1);
                    if (r == FS_EXFAT) {
                        printk("-----------------\nexFAT partition found.\n-----------------\n");
                    }
                }
                break;
            }
        }
        if (j == N_KNOWN_GUID) {
            printk("GPT (sd%d,p%d): unknown guid.\n", did, i);
        }
    }
}

int
init_mbr(int did)
{
    int i, blk = 0;
    if (disk[did].driver_type == DISK_SATA) {
        sata_read_block(did, blk);
    } else {
        panic("unknown disk type.\n");
    }
    struct MBR *mbr = (struct MBR *)blk2kaddr(did, blk);
    for (i = 0; i < 4; ++i) {
        int unknown = 0, part_type = mbr->part[i].part_type;
        switch (part_type) {
        case 0x00:
            printk("MBR part %d: empty.\n", i);
            break;
        case 0x01:
            printk("MBR part %d: FAT12.\n", i);
            break;
        case 0x04:
            printk("MBR part %d: FAT16.\n", i);
            break;
        case 0x06:
            printk("MBR part %d: FAT16B.\n", i);
            break;
        case 0x07:
            printk("MBR part %d: NTFS/exFAT\n", i);
            break;
        case 0x0B:
            printk("MBR part %d: FAT32(CHS)\n", i);
            break;
        case 0x0C:
            printk("MBR part %d: FAT32(LBA)\n", i);
            break;
        case 0x0F:
            printk("MBR part %d: extended part with LBA\n", i);
            break;
        case 0x82:
            printk("MBR part %d: linux swap\n", i);
            break;
        case 0x83:
            printk("MBR part %d: linux fs\n", i);
            break;
        case 0xEE:
            printk("MBR part %d: GPT\n", i);
            init_gpt(did);
            break;
        case 0xEF:
            printk("MBR part %d: EFI\n", i);
            break;
        default:
            printk("MBR part %d: unknown part type: %d\n", i, part_type);
            unknown = 1;
            break;
        }
        if (!unknown && part_type != 0xEE) {
            int pid = disk[did].n_part++;
            disk[did].part[pid].lba_beg = mbr->part[i].lba_start;
            disk[did].part[pid].n_sec = mbr->part[i].n_sec;
            disk[did].part[pid].part_type = part_type;
        }
    }
}

int
print_guid(char *guid)
{
    int i;
    for (i = 0; i < 16; ++i) {
        uint8_t lo = guid[i] & 0xF, hi = (uint8_t)guid[i] >> 4;
        printk("%c%c ", "0123456789ABCDEF"[hi], "0123456789ABCDEF"[lo]);
    }
    return 0;
}

int
detect_fs(int did, int pid)
{
    sata_read_block(did, disk[did].part[pid].lba_beg / 8);
    struct exFAT_hdr *fat_hdr = (struct exFAT_hdr *)blk2kaddr(did, disk[did].part[pid].lba_beg / 8);
    if (strcmp(fat_hdr->fs_name, "EXFAT   ", 8) == 0) {
        return FS_EXFAT;
    } else {
        return FS_UNKNOWN;
    }
}