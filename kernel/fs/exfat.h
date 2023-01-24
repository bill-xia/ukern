#ifndef EXFAT_H
#define EXFAT_H

#include "types.h"
#include "fs/disk.h"

#define NO_FAT_CHAIN 0x02

struct exFAT_hdr {
	u8 jmp_boot[3];
	u8 fs_name[8];
	u8 zero[53];
	u64 partition_offset;
	u64 volume_length;
	u32 fat_offset;
	u32 fat_length;
	u32 cluster_heap_offset;
	u32 cluster_count;
	u32 rtdir_cluster;
	u32 vol_serial_num;
	u16 fs_rev;
	u16 vol_flags;
	u8 byte_per_sec_shift;
	u8 sec_per_clus_shift;
	u8 num_of_fats;
	u8 drive_select;
	u8 percent_in_use;
	u8 reserved[7];
	u8 code[390];
	u16 sign;
} __attribute__((packed));

struct FS_exFAT {
	struct diskpart_t *part;
	struct exFAT_hdr *hdr;
	u32 *fat;
	int did;
};

struct dir_entry {
	u8 entry_type;
	u8 custom[19];
	u32 first_clus;
	u64 data_len;
}__attribute__((packed));

struct file_dir_entry {
	u8 entry_type;
	u8 secondary_count;
	u16 set_checksum;
	u16 file_attr;
	u16 res1;
	u32 create_ts;
	u32 last_mod_ts;
	u8 create_10ms;
	u8 last_mod_10ms;
	u8 create_utf_offset;
	u8 last_mod_utf_offset;
	u8 last_acc_utf_offset;
	u8 res2[7];
}__attribute__((packed));

struct stream_ext_entry {
	u8 entry_type;
	u8 secondary_flags;
	u8 res1;
	u8 name_len;
	u16 name_hash;
	u16 res2;
	u64 valid_data_len;
	u32 res3;
	u32 first_clus;
	u64 data_len;
}__attribute__((packed));

struct file_name_entry {
	u8 entry_type;
	u8 secondary_flags;
	u16 file_name[15];
}__attribute__((packed));

void init_fs_exfat(struct FS_exFAT *fs);
int exfat_open_file(struct FS_exFAT *fs, const char *filename, struct file_desc *fdesc);
int exfat_read_file(struct FS_exFAT *fs, char *dst, size_t sz, struct file_desc *fdesc);

#endif