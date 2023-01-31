#ifndef FS_H
#define FS_H

#include "mem.h"
#include "disk.h"

struct file_meta_exfat {
	u32	head_cluster, // 0/1 means empty file desc, otherwise in use
		use_fat;
};

struct file_meta_ext2 {
	u32	rsv;
};

struct file_desc {
	u64	file_len,
		read_ptr;
	union {
		struct file_meta_exfat	meta_exfat;
		struct file_meta_ext2	meta_ext2;
	};
	u8	inuse;
};

enum fs_type {
	FS_UNKNOWN,
	FS_EXFAT,
	N_KNOWN_FS
};

int detect_fs(int did, int pid);

int read_file(char *dst, size_t sz, struct file_desc *fdesc);
int open_file(const char *filename, struct file_desc *fdesc);

#define DSKSHIFT	41

static inline u64
blk2kaddr(int did, u64 blk)
{
	return KDISK | ((u64)did << DSKSHIFT) | (blk << PGSHIFT);
}

static inline u64
lba2kaddr(int did, u64 lba)
{
	return KDISK | ((u64)did << DSKSHIFT) | (lba << (PGSHIFT - 3));
}

#endif