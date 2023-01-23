#ifndef DISKFMT_H
#define DISKFMT_H

#include "types.h"

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

int a = sizeof(struct MBR);

#endif