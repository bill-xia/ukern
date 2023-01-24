#ifndef FS_H
#define FS_H

#define E_TRAVEL_INTO_FILE      1
#define E_FILE_NOT_EXIST        2
#define E_FILE_NAME_TOO_LONG    3
#define E_NO_AVAIL_FD           4

#define E_INVALID_FD    1
#define E_FD_NOT_OPENED 2
#define E_INVALID_MEM   3

#include "mem.h"

struct file_meta_exfat {
    uint32_t head_cluster, // 0/1 means empty file desc, otherwise in use
             use_fat;
};

struct file_meta_ext2 {
    uint32_t rsv;
};

struct file_desc {
    uint64_t file_len, read_ptr;
    union {
        struct file_meta_exfat meta_exfat;
        struct file_meta_ext2 meta_ext2;
    };
    uint8_t inuse;
};

struct diskpart_t {
    char name[32];
    uint8_t part_type,
            fs_type;
    uint64_t    lba_beg,
                n_sec;
};

enum driver_type {
    DISK_UNKNOWN,
    DISK_SATA
};

struct disk_t {
    char name[32];
    uint8_t driver_type,
            disk_ind,   // index among the driver type
                        // e.g. sata port 0 has disk_ind 0,
                        // an nvme disk may also has disk_ind 0
            n_part;
    struct diskpart_t part[128];
};

extern int n_disk;
extern struct disk_t disk[32];

enum fs_type {
    FS_UNKNOWN,
    FS_EXFAT,
    N_KNOWN_FS
};

int detect_fs(int did, int pid);
int disk_read(int did, uint64_t lba, int n_sec);

int read_file(char *dst, size_t sz, struct file_desc *fdesc);
int open_file(const char *filename, struct file_desc *fdesc);

#define DSKSHIFT 40

static uint64_t
blk2kaddr(int did, uint64_t blk)
{
    return KDISK | ((uint64_t)did << DSKSHIFT) | (blk << PGSHIFT);
}

static uint64_t
lba2kaddr(int did, uint64_t lba)
{
    return KDISK | ((uint64_t)did << DSKSHIFT) | (lba << (PGSHIFT - 3));
}

#endif