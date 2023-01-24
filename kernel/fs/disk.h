#ifndef DISK_H
#define DISK_H

/**
 * disk:
 * Provide a unified interface to read from/write to SATA disk and NVMe disk,
 * and recognize partitions on the disk.
 */

#include "types.h"
#include "string.h"
#include "fs.h"

#define PARTTYPE_GPT 0x10000

struct guid {
	u32	f1;
	u16	f2,
		f3;
	u8	f4[8];
};

static int
guid_eq(struct guid *a, struct guid *b)
{
	return !strcmp((char *)a, (char *)b, 16);
}

struct disk_part_entry {
	u8	status,
		head_start,
		sector_start,
		cylinder_start,
		part_type,
		head_end,
		sector_end,
		cylinder_end;
	u32	lba_start,
		n_sec;
} __attribute__((packed));

struct MBR {
	char	boot_code[440];
	u32	disk_sig;
	u16	disk_sig2;
	struct disk_part_entry	part[4];
	u16	boot_sig;
} __attribute__((packed));

struct GPT {
	u64		sign;
	u32		rev,
			hdr_siz,
			crc_hdr,
			rsv;
	u64		cur_lba,
			backup_lba,
			lba_beg,
			lba_end;
	struct guid	guid;
	u64		lba_pararr;
	u32		n_pararr,
			par_entry_siz,
			crc_pararr;
	char		rsv2[0];
} __attribute__((packed));

struct GPTPAR {
	struct guid	type_guid;
	struct guid	uniq_guid;
	u64		lba_beg,
			lba_end,
			attr_flags;
	u16		part_name[36]; // UTF-16
} __attribute__((packed));


struct diskpart_t {
	char	name[32];
	u8	part_type,
		fs_type;
	u64	lba_beg,
		n_sec;
};

enum driver_type {
	DISK_UNKNOWN,
	DISK_SATA
};

struct disk_t {
	char	name[32];
	u8	driver_type,
		disk_ind,	// index among the driver type
				// e.g. sata port 0 has disk_ind 0,
				// an nvme disk may also has disk_ind 0
		n_part;
	struct diskpart_t	part[128];
};

extern int	n_disk;
extern struct disk_t	disk[32];

int init_disk(void);
int disk_read(int did, u64 lba, int n_sec);

#endif