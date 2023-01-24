# FS

This is the file system module.

## Disk

It's implemented in `disk.c` and `disk.h`.

### Terms

- sector: a 512B storage unit on the disk
- block: a 4K storage unit on the disk / in the memory

### Overview

`disk` module provides a unified interface to read from/write to SATA disk and NVMe disk, and recognizes partitions on the disk.

`init_disk()` executes after PCIe initialization, because it depends on the disk hardware information that is found in PCIe.

Each disk has a corresponding disk_t structure, stored in global array `disk[]`, and global variable `n_disk` records disk number. They're (partly) initailized during PCIe enumeration. Then in `init_disk()`, partition information are filled in.

Memory space `[KDISK, KDISK+(1 << 44))` are used for disk cache, i.e. temporarily stores disk content inside memory. Each disk takes `(1<<41)B/2TB` of address space, thus ukern supports up to 8 disks with each disk's capacity less than 2TB.

### API

TODO: return value.

- `init_disk()`: init all the detected disk, fill their partition information in `disk[]`, and init root file system inside if possible.
- `disk_read(int did, u64 lba, int n_sec)`: read sectors `[lba, lba+n_sec)` from the `did`-th disk
  > Now we call this function to read from disk manually. Maybe this will be done in page fault handler later, and we no longer need to call them actively, which is a neat way.

## fs

It's implemented in `fs.c` and `fs.h`.

### Overview

`fs` module provides a unified interface to interact with multiple different file systems. Although only exFAT is (partly) implemented now, the design should be extensible to other file systems. `detect_fs()` also sits here, which is called by `init_disk()` when it founds a partiion that may have a file system.

Allowed operations are `open()` and `read()`, which are quite straight forward. `write()` and `close()` is on the to-do list. These operations are implemented in each file system's own module, fs module only act like a dispatcher here.

File descriptor consists of several common properties, such as file length and read pointer, and a union of FS-specific metadata. This design helps to maintain a unified interface to multiple FS.

### API

TODO: return value.

- `detect_fs(int did, int pid)`: detect file system inside the pid-th partition of the did-th disk. Initialize it if it's the root filesystem.
- `open_file(const char *filename, struct file_desc *fdesc)`: open file `filename`, fill in fdesc.
- `read_file(char *dst, size_t sz, struct file_desc *fdesc)`: read sz bytes into dst from the file described by fdesc.

## exFAT

It's implemented in `exfat.c` and `exfat.h`.

### Overview

`exFAT` module implements the exFAT file system. Specifically, it exposes a function `init_fs_exfat()` to initailize the file system, which is called by `detect_fs()`, and its `open()` and `read()` implementation.

### API

TODO: return value.

- `init_fs_exfat(struct FS_exFAT *fs)`: initialize fs, use disk and partition information in fs, fill exfat-specific information back to fs, e.g. exFAT header pointer and FAT pointer.
- `exfat_open_file(struct FS_exFAT *fs, const char *filename, struct file_desc *fdesc)`: open file `filename` in exFAT file system `fs`, fill in fdesc.
- `exfat_read_file(struct FS_exFAT *fs, char *dst, size_t sz, struct file_desc *fdesc)`: read sz bytes into dst from the file described by fdesc, which must sit in a exFAT file system.