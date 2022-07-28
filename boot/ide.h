#ifndef IDE_H
#define IDE_H

#ifdef __GNUC__
#define Packed __attribute__((packed));
#else
#define Packed /* nothing */
#endif

#define IDE_EOT 128

#define IDE_CMD_START 1
#define IDE_CMD_RW 8

#define IDE_STAT_ACTIVE 1
#define IDE_STAT_INTR 4

struct phys_region_desc {
    unsigned base_addr;
    short byte_cnt;
    char reserved[3];
    char eot;
} Packed;

#endif