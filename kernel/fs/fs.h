#ifndef FS_H
#define FS_H

#include "types.h"

#define E_TRAVEL_INTO_FILE      1
#define E_FILE_NOT_EXIST        2
#define E_FILE_NAME_TOO_LONG    3
#define E_NO_AVAIL_FD           4

#define E_INVALID_FD    1
#define E_FD_NOT_OPENED 2
#define E_INVALID_MEM   3

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
};

struct file_desc {
    uint32_t head_cluster, // 0/1 means empty file desc, otherwise in use
             use_fat;
    uint64_t file_len, read_ptr;
};

struct dir_entry {
    uint8_t entry_type;
    uint8_t custom[19];
    uint32_t first_clus;
    uint64_t data_len;
};

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
};

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
};

struct file_name_entry {
    uint8_t entry_type;
    uint8_t secondary_flags;
    uint16_t file_name[15];
};

void init_fs();
uint32_t get_fat_at(uint32_t id);
int open_file(const char *filename, uint32_t *head_cluster, uint64_t *file_len, uint32_t *use_fat);
int read_file(uint32_t clus_id, uint64_t ptr, char *dst, uint32_t sz, uint32_t use_fat);

extern struct exFAT_hdr *fsinfo;

#endif