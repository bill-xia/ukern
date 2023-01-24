#include "fs/exfat.h"
#include "fs/fs.h"
#include "mem.h"
#include "fs/ide.h"
#include "printk.h"

uint32_t *fat;
uint8_t fs_buf[4096];

void
init_fs_exfat(struct FS_exFAT *fs)
{
    fs->hdr = (struct exFAT_hdr *)lba2kaddr(fs->did, fs->part->lba_beg);
    fat = (uint32_t *)lba2kaddr(fs->did, fs->part->lba_beg + fs->hdr->fat_offset);
    // printk("init fs exfat, fat %p, sec_per_clus_shift %d\n", fat, fs->hdr->sec_per_clus_shift);
}

uint32_t
get_fat_at(struct FS_exFAT *fs, uint32_t id)
{
    // printk("get_fat_at(%x), lba_beg %d, fat_offset %d, fat=",
    //     id, fs->part->lba_beg, fs->hdr->fat_offset);
    disk_read(fs->did, fs->part->lba_beg + fs->hdr->fat_offset + ((id * 4) >> 9), 1);
    printk("%x\n", fat[id]);
    return fat[id];
}

uint8_t
to_upper_case(uint8_t c)
{
    if ('a' <= c && c <= 'z') c += 'A' - 'a';
    return c;
}

int
cmp_fn(uint8_t c1, uint16_t c2) {
    // printk("cmp_fn(%c,%c):%x  ",c1,c2,to_upper_case(c1) != to_upper_case(c2));
    return to_upper_case(c1) != to_upper_case(c2);
}

// open file identified by `filename`
// pass the head cluster id to *head_cluster
// return status: 0 if success, < 0 if error
int
exfat_open_file(struct FS_exFAT *fs, const char *filename, struct file_desc *fdesc)
{
    uint32_t    dir_clus_id,
                clus_id = 0,
                nxt_level_clus_id = fs->hdr->rtdir_cluster,
                isdir = 1,
                secondary_count,
                fn_len,
                data_len,
                ptr,
                keep_cmp_fn,
                matched,
                cur_use_fat,
                nxt_use_fat = 1;
    static char name[256];
    static struct dir_entry _dir[128]; // 4K
    // TODO: what if cluster size > 4K?
    int entry_perclus = (512 << (fs->hdr->sec_per_clus_shift)) / sizeof(struct dir_entry);
    // printk("%d, entry_perclus: %d\n", (512 << (fs->hdr->sec_per_clus_shift)), entry_perclus);
    for (int i = 0, ind = 0; i < 256; ++i, ++ind) {
        if (filename[i] != '/' && filename[i] != '\0') {
            name[ind] = filename[i];
        } else {
            if (ind == 0) {
                goto match_end;
            }
            if (!isdir) {
                // travel into a file, like "/a/b" where "/a" is a file
                return -E_TRAVEL_INTO_FILE;
            }
            dir_clus_id = nxt_level_clus_id;
            nxt_level_clus_id = 0;
            cur_use_fat = nxt_use_fat;
            nxt_use_fat = 0;
            // start matching the name
            matched = 0;
            for (int j = 0;; ++j) {
                if (j % entry_perclus == 0) {
                    if (dir_clus_id == 0xFFFFFFFF) break;
                    disk_read(
                        fs->did,
                        fs->part->lba_beg + fs->hdr->cluster_heap_offset + ((dir_clus_id - 2) << fs->hdr->sec_per_clus_shift),
                        1
                    );
                    memcpy(_dir, (void *)lba2kaddr(fs->did, fs->part->lba_beg + fs->hdr->cluster_heap_offset + ((dir_clus_id - 2) << fs->hdr->sec_per_clus_shift)), (512 << (fs->hdr->sec_per_clus_shift)));
                    // printk("dir_clus_id: %x\n", dir_clus_id);
                    if (cur_use_fat) {
                        // printk("open(): ");
                        dir_clus_id = get_fat_at(fs, dir_clus_id);
                    } else
                        dir_clus_id++;
                }
                switch (_dir[j % entry_perclus].entry_type) {
                case 0x85: ;// file_dir
                    struct file_dir_entry *fd_dir = (struct file_dir_entry *)&_dir[j % entry_perclus];
                    secondary_count = fd_dir->secondary_count;
                    keep_cmp_fn = 1;
                    ptr = 0;
                    isdir = (fd_dir->file_attr & 0x10) >> 4; // Directory
                    break;
                case 0xC0: ;// stream_ext
                    struct stream_ext_entry *str_ext_dir = (struct stream_ext_entry *)&_dir[j % entry_perclus];
                    clus_id = str_ext_dir->first_clus;
                    fn_len = str_ext_dir->name_len;
                    data_len = str_ext_dir->valid_data_len;
                    nxt_use_fat = !(str_ext_dir->secondary_flags & NO_FAT_CHAIN); 
                    break;
                case 0xC1: ;// file_name
                    struct file_name_entry *fn_dir = (struct file_name_entry *)&_dir[j % entry_perclus];
                    for (int k = 0; keep_cmp_fn && k < 15 && ptr < fn_len && ptr < ind; k++) {
                        if (cmp_fn(name[ptr++], fn_dir->file_name[k])) {
                            keep_cmp_fn = 0;
                        }
                    }
                    if (keep_cmp_fn && ptr == fn_len && ptr == ind) {
                        // matched!
                        matched = 1;
                        nxt_level_clus_id = clus_id;
                    }
                    break;
                default:
                    break;
                }
                if (matched) {
                    break;
                }
            }
        match_end:
            if (filename[i] == '\0') {
                if (nxt_level_clus_id == 0) {
                    return -E_FILE_NOT_EXIST; // file not exist
                }
                fdesc->meta_exfat.head_cluster = clus_id;
                fdesc->meta_exfat.use_fat = nxt_use_fat;
                fdesc->file_len = data_len;
                fdesc->read_ptr = 0;
                return 0;
            }
            ind = -1;
        }
    }
    return -E_FILE_NAME_TOO_LONG;
}

inline uint64_t
min(uint64_t a, uint64_t b)
{
    return a < b ? a : b;
}

int
exfat_read_file(struct FS_exFAT *fs, char *dst, size_t sz, struct file_desc *fdesc)
{
    int r = 0,
        ptr = fdesc->read_ptr,
        clus_id = fdesc->meta_exfat.head_cluster,
        use_fat = fdesc->meta_exfat.use_fat;
    // printk("read(): ptr %d, clusid %d, file_len %d, sz %d\n", ptr, clus_id, fdesc->file_len, sz);
    while (ptr >> (9 + fs->hdr->sec_per_clus_shift)) {
        ptr -= (1 << (9 + fs->hdr->sec_per_clus_shift));
        // printk("read(): ");
        if (fdesc->meta_exfat.use_fat)
            clus_id = get_fat_at(fs, clus_id);
        else
            clus_id++;
    }
    sz = min(sz, fdesc->file_len - fdesc->read_ptr);
    while (sz) {
        disk_read(
            fs->did,
            fs->part->lba_beg + fs->hdr->cluster_heap_offset + ((clus_id - 2) << fs->hdr->sec_per_clus_shift),
            1
        );
        uint64_t src = lba2kaddr(fs->did, fs->part->lba_beg + fs->hdr->cluster_heap_offset + ((clus_id - 2) << fs->hdr->sec_per_clus_shift)) + ptr;
        int n = min(sz, PGSIZE - (src % PGSIZE));
        // printk("dst %p, src %p, sz %d, sz2 %d\n", dst, src, sz, PGSIZE - (src % PGSIZE));
        memcpy(
            dst,
            (void *)src,
            n
        );
        sz -= n;
        dst += n;
        ptr = 0;
        while (n >> (9 + fs->hdr->sec_per_clus_shift)) {
            n -= (1 << (9 + fs->hdr->sec_per_clus_shift));
            // printk("read(): ");
            if (fdesc->meta_exfat.use_fat)
                clus_id = get_fat_at(fs, clus_id);
            else
                clus_id++;
        }
    }
    return r;
}