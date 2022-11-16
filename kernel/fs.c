#include "fs.h"
#include "mem.h"
#include "ide.h"
#include "printk.h"

struct exFAT_hdr *fsinfo;
uint32_t *fat;
uint8_t *fs_buf;

void
init_fs()
{
    struct PageInfo *page = alloc_page(FLAG_ZERO);
    if (page == NULL) {
        printk("init_fs(): no page available.\n");
        while (1);
    }
    fsinfo = (struct exFAT_hdr *)P2K(page->paddr);
    page = alloc_page(FLAG_ZERO);
    if (page == NULL) {
        printk("init_fs(): no page available.\n");
        while (1);
    }
    fat = (uint32_t *)P2K(page->paddr);
    page = alloc_page(FLAG_ZERO);
    if (page == NULL) {
        printk("init_fs(): no page available.\n");
        while (1);
    }
    fs_buf = (uint8_t *)P2K(page->paddr);
    // read MBR block
    ide_read(0, fsinfo, 1);
}

uint32_t
get_fat_at(uint32_t id)
{
    // printk("get_fat_at(%x)\n", id);
    ide_read(fsinfo->fat_offset + (id >> 7), fat, 1);
    return fat[id & 127];
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
open_file(const char *filename, uint32_t *head_cluster, uint64_t *file_len, uint32_t *use_fat)
{
    uint32_t    dir_clus_id,
                clus_id = 0,
                nxt_level_clus_id = fsinfo->rtdir_cluster,
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
    static struct dir_entry _dir[16];
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
                if (j % 16 == 0) {
                    if (dir_clus_id == 0xFFFFFFFF) break;
                    ide_read(fsinfo->cluster_heap_offset + dir_clus_id - 2, _dir, 1);
                    // printk("dir_clus_id: %x\n", dir_clus_id);
                    if (cur_use_fat)
                        dir_clus_id = get_fat_at(dir_clus_id);
                    else
                        dir_clus_id++;
                }
                switch (_dir[j % 16].entry_type) {
                case 0x85: ;// file_dir
                    struct file_dir_entry *fd_dir = (struct file_dir_entry *)&_dir[j % 16];
                    secondary_count = fd_dir->secondary_count;
                    keep_cmp_fn = 1;
                    ptr = 0;
                    isdir = (fd_dir->file_attr & 0x10) >> 4; // Directory
                    break;
                case 0xC0: ;// stream_ext
                    struct stream_ext_entry *str_ext_dir = (struct stream_ext_entry *)&_dir[j % 16];
                    clus_id = str_ext_dir->first_clus;
                    fn_len = str_ext_dir->name_len;
                    data_len = str_ext_dir->valid_data_len;
                    nxt_use_fat = !(str_ext_dir->secondary_flags & NO_FAT_CHAIN); 
                    break;
                case 0xC1: ;// file_name
                    struct file_name_entry *fn_dir = (struct file_name_entry *)&_dir[j % 16];
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
                *head_cluster = clus_id;
                *file_len = data_len;
                *use_fat = nxt_use_fat;
                return 0;
            }
            ind = -1;
        }
    }
    return -E_FILE_NAME_TOO_LONG;
}

int
read_file(uint32_t clus_id, uint64_t ptr, char *dst, uint32_t sz, uint32_t use_fat)
{
    int r = 0;
    while (ptr >> 9) {
        ptr -= 512;
        clus_id = get_fat_at(clus_id);
    }
    ide_read(fsinfo->cluster_heap_offset - 2 + clus_id, fs_buf, 1);
    char *d = dst;
    while (sz--) {
        *dst = fs_buf[ptr];
        ptr++;
        r++;
        dst++;
        if (ptr == 512) {
            if (use_fat)
                clus_id = get_fat_at(clus_id);
            else
                clus_id++;
            ide_read(fsinfo->cluster_heap_offset - 2 + clus_id, fs_buf, 1);
            ptr = 0;
        }
    }
    return r;
}