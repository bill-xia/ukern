#ifndef EXFAT_H
#define EXFAT_H

#include "types.h"
#include "fs/diskfmt.h"

#define NO_FAT_CHAIN 0x02

struct exFAT_hdr {
    uint8_t jmp_boot[3];
    uint8_t fs_name[8];
    uint8_t zero[53];
    uint64_t partition_offset;
    uint64_t volume_length;
    uint32_t fat_offset;
    uint32_t fat_length;
    uint32_t cluster_heap_offset;
    uint32_t cluster_count;
    uint32_t rtdir_cluster;
    uint32_t vol_serial_num;
    uint16_t fs_rev;
    uint16_t vol_flags;
    uint8_t byte_per_sec_shift;
    uint8_t sec_per_clus_shift;
    uint8_t num_of_fats;
    uint8_t drive_select;
    uint8_t percent_in_use;
    uint8_t reserved[7];
    uint8_t code[390];
    uint16_t sign;
} __attribute__((packed));

struct FS_exFAT {
    struct diskpart_t *part;
    struct exFAT_hdr *hdr;
    int did;
};

struct dir_entry {
    uint8_t entry_type;
    uint8_t custom[19];
    uint32_t first_clus;
    uint64_t data_len;
}__attribute__((packed));

struct file_dir_entry {
    uint8_t entry_type;
    uint8_t secondary_count;
    uint16_t set_checksum;
    uint16_t file_attr;
    uint16_t res1;
    uint32_t create_ts;
    uint32_t last_mod_ts;
    uint8_t create_10ms;
    uint8_t last_mod_10ms;
    uint8_t create_utf_offset;
    uint8_t last_mod_utf_offset;
    uint8_t last_acc_utf_offset;
    uint8_t res2[7];
}__attribute__((packed));

struct stream_ext_entry {
    uint8_t entry_type;
    uint8_t secondary_flags;
    uint8_t res1;
    uint8_t name_len;
    uint16_t name_hash;
    uint16_t res2;
    uint64_t valid_data_len;
    uint32_t res3;
    uint32_t first_clus;
    uint64_t data_len;
}__attribute__((packed));

struct file_name_entry {
    uint8_t entry_type;
    uint8_t secondary_flags;
    uint16_t file_name[15];
}__attribute__((packed));

void init_fs_exfat(struct FS_exFAT *fs);
uint32_t get_fat_at(struct FS_exFAT *fs, uint32_t id);
int exfat_open_file(struct FS_exFAT *fs, const char *filename, struct file_desc *fdesc);
int exfat_read_file(struct FS_exFAT *fs, char *dst, size_t sz, struct file_desc *fdesc);

#endif