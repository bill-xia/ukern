#include "diskfmt.h"
#include "mem.h"
#include "pcie/sata.h"
#include "printk.h"

int n_disk;
struct disk_t disk[32];
struct guid known_guid[] = {
    {0,0,0,{0}}, // empty
    {0xC12A7328, 0xF81F, 0x11D2, {0xBA, 0x4B, 0x00, 0xA0, 0xC9, 0x3E, 0xC9, 0x3B}}, // EFI
    {0xEBD0A0A2, 0xB9E5, 0x4433, {0x87, 0xC0, 0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7}}, // Windows Basic Data
    {0x0FC63DAF, 0x8483, 0x4772, {0x8E, 0x79, 0x3D, 0x69, 0xD8, 0x47, 0x7D, 0xE4}}, // Linux FS
};

#define N_KNOWN_GUID (sizeof(known_guid) / sizeof(struct guid))

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
    int i, blk;
    if (disk[did].driver_type == DISK_SATA) {
        for (blk = 1; blk <= 5; ++blk) {
            walk_pgtbl(k_pgtbl, blk2kaddr(blk), NULL, 1);
            sata_read_block(disk[did].disk_ind, blk);
        }
    } else {
        panic("unknown disk type.\n");
    }
    struct GPT *gpt = (struct GPT *)(blk2kaddr(0) + 512);
    printk("GPT sig: %lx, lbabeg: %ld, lbaend: %ld, parlba: %ld, n_par: %d, entrysiz: %d\n",
        gpt->sign, gpt->lba_beg, gpt->lba_end, gpt->lba_pararr, gpt->n_pararr, gpt->par_entry_siz);
    char *ptr = (char *)gpt + 512;
    int n_part = gpt->n_pararr, par_entry_siz = gpt->par_entry_siz;
    printk("GPT npar: %d\n", n_part);
    for (i = 0; i < n_part; ++i, ptr += par_entry_siz) {
        struct GPTPAR *par = (struct GPTPAR *)ptr;
        int j;
        for (j = 0; j < N_KNOWN_GUID; ++j) {
            if (guid_eq(&par->type_guid, &known_guid[j])) {
                if (j == 0)
                    break;
                disk[did].part[disk[did].n_part++].part_type = PARTTYPE_GPT | j;
                print_guid(&par->type_guid);
                break;
            }
        }
        if (j == N_KNOWN_GUID) {
            printk("(sd%d,p%d): unknown guid.\n", did, i);
        }
    }
}

int
init_mbr(int did)
{
    int i, blk = 0;
    if (disk[did].driver_type == DISK_SATA) {
        walk_pgtbl(k_pgtbl, blk2kaddr(blk), NULL, 1);
        sata_read_block(disk[did].disk_ind, blk);
    } else {
        panic("unknown disk type.\n");
    }
    struct MBR *mbr = (struct MBR *)blk2kaddr(blk);
    for (i = 0; i < 4; ++i) {
        int unknown = 0, part_type = mbr->part[i].part_type;
        switch (part_type) {
        case 0x00:
            printk("part %d: empty.\n", i);
            break;
        case 0x01:
            printk("part %d: FAT12.\n", i);
            break;
        case 0x04:
            printk("part %d: FAT16.\n", i);
            break;
        case 0x06:
            printk("part %d: FAT16B.\n", i);
            break;
        case 0x07:
            printk("part %d: NTFS/exFAT\n", i);
            break;
        case 0x0B:
            printk("part %d: FAT32(CHS)\n", i);
            break;
        case 0x0C:
            printk("part %d: FAT32(LBA)\n", i);
            break;
        case 0x0F:
            printk("part %d: extended part with LBA\n", i);
            break;
        case 0x82:
            printk("part %d: linux swap\n", i);
            break;
        case 0x83:
            printk("part %d: linux fs\n", i);
            break;
        case 0xEE:
            printk("part %d: GPT\n", i);
            init_gpt(did);
            break;
        case 0xEF:
            printk("part %d: EFI\n", i);
            break;
        default:
            printk("part %d: unknown part type: %d\n", i, part_type);
            unknown = 1;
            break;
        }
        if (!unknown && part_type != 0xEE) {
            disk[did].part[disk[did].n_part++].part_type = part_type;
        }
    }
}

int
print_guid(char *guid)
{
    static int n_guid = 0;
    int i;
    n_guid++;
    if (n_guid >= 10) {
        return;
    }
    printk("known GUID: ");
    for (i = 0; i < 16; ++i) {
        uint8_t lo = guid[i] & 0xF, hi = (uint8_t)guid[i] >> 4;
        printk("%c%c ", "0123456789ABCDEF"[hi], "0123456789ABCDEF"[lo]);
    }
    printk("\n");
}