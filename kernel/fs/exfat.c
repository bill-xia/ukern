#include "fs/exfat.h"
#include "fs/fs.h"
#include "mem.h"
#include "printk.h"
#include "errno.h"

void
init_fs_exfat(struct FS_exFAT *fs)
{
	fs->hdr = (struct exFAT_hdr *)lba2kaddr(fs->did, fs->part->lba_beg);
	fs->fat = (u32 *)lba2kaddr(fs->did, fs->part->lba_beg + fs->hdr->fat_offset);
	printk("init fs exfat, fat %p, part_lbabeg %ld, fat_offset %d, sec_per_clus_shift %d, bytes_per_sec_shift %d\n",
		fs->fat,
		fs->part->lba_beg,
		fs->hdr->fat_offset,
		(u32)fs->hdr->sec_per_clus_shift,
		(u32)fs->hdr->byte_per_sec_shift);
}

u32
get_fat_at(struct FS_exFAT *fs, u32 id)
{
	// printk("get_fat_at(%x), lba_beg %d, fat_offset %d, fat=",
	//     id, fs->part->lba_beg, fs->hdr->fat_offset);
	disk_read(fs->did, fs->part->lba_beg + fs->hdr->fat_offset + ((id * 4) >> 9), 1);
	// printk("%x\n", fs->fat[id]);
	return fs->fat[id];
}

u8
to_upper_case(u8 c)
{
	if ('a' <= c && c <= 'z') c += 'A' - 'a';
	return c;
}

int
cmp_fn(u8 c1, u16 c2) {
	// printk("cmp_fn(%c,%c):%x  ",c1,c2,to_upper_case(c1) != to_upper_case(c2));
	return to_upper_case(c1) != to_upper_case(c2);
}

struct exfat_dir_file {
	u32	clus_id,
		use_fat:1, is_dir:1;
	u64	file_len;
};

/**
 * exfat_walk_dir() - find dir/file with `name` inside cur_dir, and walk into it
 * 	(i.e. change cur_dir to point to the dir/file found)
 * 
 * return 0 on success, -1 if `name` doesn't exist
 */
int
exfat_walk_dir(struct FS_exFAT *fs, const char *name, int name_len, struct exfat_dir_file *cur_dir)
{
	struct dir_entry *dir;
	// TODO: what if cluster size > 4K?

	int	name_ptr,
		secondary_count,
		name_dismatch,
		clus_size = (1ul << (fs->hdr->byte_per_sec_shift + fs->hdr->sec_per_clus_shift)),
		entry_perclus = clus_size / sizeof(struct dir_entry),
		clus_id,
		use_fat,
		fn_len,
		is_dir,
		matched = 0;
	u64	data_len;
	for (int i = 0;; ++i) {
		// cur->clus_id stores the clus *to be read*, rather than already read
		// Thus, first read, then update clus_id
		if (i % entry_perclus == 0) {
			if (cur_dir->clus_id == 0xFFFFFFFF)
				break;
			disk_read(fs->did, EXFAT_CLUS2LBA(fs, cur_dir->clus_id), 1u << fs->hdr->sec_per_clus_shift);
			dir = (struct dir_entry *)lba2kaddr(fs->did, EXFAT_CLUS2LBA(fs, cur_dir->clus_id));
			// printk("dir_clus_id: %x\n", dir_clus_id);
			if (cur_dir->use_fat) {
				// printk("open(): ");
				cur_dir->clus_id = get_fat_at(fs, cur_dir->clus_id);
			} else {
				cur_dir->clus_id++;
			}
			
		}
		switch (dir[i % entry_perclus].entry_type) {
		case 0x85: ;// file_dir
			struct file_dir_entry *fd_dir = (struct file_dir_entry *)&dir[i % entry_perclus];
			secondary_count = fd_dir->secondary_count;
			name_dismatch = 0;
			name_ptr = 0;
			is_dir = (fd_dir->file_attr & 0x10) >> 4; // Directory
			break;
		case 0xC0: ;// stream_ext
			struct stream_ext_entry *str_ext_dir = (struct stream_ext_entry *)&dir[i % entry_perclus];
			clus_id = str_ext_dir->first_clus;
			fn_len = str_ext_dir->name_len;
			data_len = str_ext_dir->valid_data_len;
			use_fat = !(str_ext_dir->secondary_flags & NO_FAT_CHAIN); 
			break;
		case 0xC1: ;// file_name
			struct file_name_entry *fn_dir = (struct file_name_entry *)&dir[i % entry_perclus];
			for (int k = 0; !name_dismatch && k < 15 && name_ptr < fn_len && name_ptr < name_len; k++) {
				if (cmp_fn(name[name_ptr++], fn_dir->file_name[k])) {
					name_dismatch = 1;
				}
			}
			if (!name_dismatch && name_ptr == fn_len && name_ptr == name_len) {
				// matched!
				cur_dir->clus_id = clus_id;
				cur_dir->use_fat = use_fat;
				cur_dir->is_dir = is_dir;
				cur_dir->file_len = data_len;
				matched = 1;
			}
			break;
		default:
			break;
		}
		if (matched)
			break;
	}
	return matched ? 0 : -1;
}

// open file identified by `filename`
// return status: 0 if success, < 0 if error
int
exfat_open_file(struct FS_exFAT *fs, const char *filename, struct file_desc *fdesc)
{
	struct exfat_dir_file cur_dir = {
		.clus_id = fs->hdr->rtdir_cluster,
		.file_len = 0,
		.is_dir = 1,
		.use_fat = 1
	};
	static char name[256];
	int  r;
	for (int i = 0, ind = 0; i < 256; ++i, ++ind) {
		if (filename[i] != '/' && filename[i] != '\0') {
			name[ind] = filename[i];
			continue;
		}
		if (ind == 0) {
			// name is empty, skip
			ind = -1;
			continue;
		}
		// `name` is complete.
		if (!cur_dir.is_dir) {
			// travel into a file, like `filename is` "/a/b" and "/a" is a file
			return -ENOTDIR;
		}
		if ((r = exfat_walk_dir(fs, name, ind, &cur_dir)) < 0) {
			return -ENOENT;
		}
		// check if filename comes to end
		if (filename[i] == '\0') {
			fdesc->meta_exfat.head_cluster = cur_dir.clus_id;
			fdesc->meta_exfat.use_fat = cur_dir.use_fat;
			fdesc->file_len = cur_dir.file_len;
			fdesc->read_ptr = 0;
			return 0;
		}
		ind = -1;
	}
	return -ENAMETOOLONG;
}

int
exfat_read_file(struct FS_exFAT *fs, char *dst, size_t sz, struct file_desc *fdesc)
{
	int	r = 0,
		ptr = fdesc->read_ptr,
		clus_id = fdesc->meta_exfat.head_cluster,
		use_fat = fdesc->meta_exfat.use_fat,
		clus_size = 1 << (fs->hdr->byte_per_sec_shift + fs->hdr->sec_per_clus_shift);
	// printk("read(): ptr %d, clusid %d, file_len %d, sz %d\n", ptr, clus_id, fdesc->file_len, sz);
	while (ptr >= clus_size) {
		ptr -= clus_size;
		// printk("read(): ");
		if (fdesc->meta_exfat.use_fat)
			clus_id = get_fat_at(fs, clus_id);
		else
			clus_id++;
	}
	sz = min(sz, fdesc->file_len - fdesc->read_ptr);
	while (sz) {
		disk_read(fs->did, EXFAT_CLUS2LBA(fs, clus_id), 1 << fs->hdr->sec_per_clus_shift);
		u64 src = lba2kaddr(fs->did, EXFAT_CLUS2LBA(fs, clus_id)) + ptr;
		int n = min(sz, clus_size - ptr);
		// printk("dst %p, src %p, sz %d, sz2 %d\n", dst, src, sz, PGSIZE - (src % PGSIZE));
		memcpy(dst, (void *)src, n);
		sz -= n;
		dst += n;
		ptr = 0;
		while (n >= clus_size) {
			n -= clus_size;
			// printk("read(): ");
			if (fdesc->meta_exfat.use_fat)
				clus_id = get_fat_at(fs, clus_id);
			else
				clus_id++;
		}
	}
	return r;
}