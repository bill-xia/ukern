#ifndef DISKFMT_H
#define DISKFMT_H

#include "types.h"
#include "string.h"
#include "fs.h"

#define PARTTYPE_GPT 0x10000

struct guid {
    uint32_t f1;
    uint16_t f2, f3;
    uint8_t f4[8];
};

static int
guid_eq(struct guid *a, struct guid *b)
{
    return !strcmp((char *)a, (char *)b, 16);
}

struct disk_part_entry {
    uint8_t status,
            head_start,
            sector_start,
            cylinder_start,
            part_type,
            head_end,
            sector_end,
            cylinder_end;
    uint32_t    lba_start,
                n_sec;
} __attribute__((packed));

struct MBR {
    char boot_code[440];
    uint32_t disk_sig;
    uint16_t disk_sig2;
    struct disk_part_entry part[4];
    uint16_t boot_sig;
} __attribute__((packed));

struct GPT {
    uint64_t    sign;
    uint32_t    rev,
                hdr_siz,
                crc_hdr,
                rsv;
    uint64_t    cur_lba,
                backup_lba,
                lba_beg,
                lba_end;
    struct guid guid;
    uint64_t    lba_pararr;
    uint32_t    n_pararr,
                par_entry_siz,
                crc_pararr;
    char rsv2[0];
} __attribute__((packed));

struct GPTPAR {
    struct guid type_guid;
    struct guid uniq_guid;
    uint64_t    lba_beg,
                lba_end,
                attr_flags;
    uint16_t    part_name[36]; // UTF-16
} __attribute__((packed));

#endif