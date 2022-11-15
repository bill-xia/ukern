#include "syscall.h"
#include "printk.h"
#include "console.h"
#include "proc.h"
#include "mem.h"
#include "x86.h"
#include "sched.h"
#include "ide.h"
#include "fs.h"

void
sys_hello(void)
{
    printk("\"hello, world!\" from user space!\n");
}

void
sys_fork(struct ProcContext *tf)
{
    struct Proc *nproc = alloc_proc();
    copy_pgtbl(nproc->pgtbl, curproc->pgtbl, CPY_PGTBL_CNTREF | CPY_PGTBL_WITHKSPACE);
    // save the "real" page table
    copy_pgtbl(nproc->p_pgtbl, curproc->pgtbl, 0);
    free_pgtbl(curproc->p_pgtbl, 0);
    curproc->p_pgtbl = (pgtbl_t)alloc_page(FLAG_ZERO)->paddr;
    copy_pgtbl(curproc->p_pgtbl, curproc->pgtbl, 0);
    // clear write flag at "write-on-copy" pages
    pgtbl_clearflags(nproc->pgtbl, PTE_W);
    pgtbl_clearflags(curproc->pgtbl, PTE_W);
    // copy context
    nproc->context = *tf;
    // set return value
    tf->rax = nproc->pid;
    nproc->context.rax = 0;
    // set state
    nproc->state = READY;
    lcr3(rcr3());
}

uint8_t to_upper_case(uint8_t c)
{
    if ('a' <= c && c <= 'z') c += 'A' - 'a';
    return c;
}

int cmp_fn(uint8_t c1, uint16_t c2) {
    printk("cmp_fn(%c,%c):%x  ",c1,c2,to_upper_case(c1) != to_upper_case(c2));
    return to_upper_case(c1) != to_upper_case(c2);
}

void
sys_open(struct ProcContext *tf)
{
    uint32_t    dir_clus_id,
                clus_id = 0,
                nxt_level_clus_id = fsinfo->rtdir_cluster,
                isdir = 1,
                secondary_count,
                fn_len,
                ptr,
                keep_cmp_fn;
    // printk("dir_clus_id: %x\n", nxt_level_clus_id);
    char *filename = tf->rdx;
    static char name[256];
    static struct dir_entry _dir[16];
    for (int i = 0, ind = 0; i < 256; ++i, ++ind) {
        if (filename[i] != '/' && filename[i] != '\0') {
            name[ind] = filename[i];
        } else {
            if (ind != 0) {
                if (!isdir) {
                    tf->rax = -4;
                    // travel into a file, like "/a/b" where "/a" is a file
                    return;
                }
                dir_clus_id = nxt_level_clus_id;
                nxt_level_clus_id = 0;
                // start matching the name
                for (int j = 0;; ++j) {
                    if (j % 16 == 0) {
                        if (dir_clus_id == 0xFFFFFFFF) break;
                        ide_read(fsinfo->cluster_heap_offset + dir_clus_id - 2, _dir, 1);
                        // printk("dir_clus_id: %x\n", dir_clus_id);
                        dir_clus_id = get_fat_at(dir_clus_id);
                    }
                    switch (_dir[j % 16].entry_type) {
                    case 0x85: ;// file_dir
                        struct file_dir_entry *fd_dir = &_dir[j % 16];
                        secondary_count = fd_dir->secondary_count;
                        keep_cmp_fn = 1;
                        ptr = 0;
                        isdir = (fd_dir->file_attr & 0x10) >> 4; // Directory
                        break;
                    case 0xC0: ;// stream_ext
                        struct stream_ext_entry *str_ext_dir = &_dir[j % 16];
                        clus_id = str_ext_dir->first_clus;
                        fn_len = str_ext_dir->name_len;
                        break;
                    case 0xC1: ;// file_name
                        struct file_name_entry *fn_dir = &_dir[j % 16];
                        for (int k = 0; keep_cmp_fn && k < 15 && ptr < fn_len; k++) {
                            if (cmp_fn(name[ptr++], fn_dir->file_name[k])) {
                                keep_cmp_fn = 0;
                            }
                        }
                        if (keep_cmp_fn && ptr == fn_len) {
                            // matched!
                            nxt_level_clus_id = clus_id;
                        }
                        break;
                    default:
                        break;
                    }
                }
            }
            if (filename[i] == '\0') {
                if (nxt_level_clus_id == 0) {
                    tf->rax = -1; // file not exist
                    return;
                }
                for (int fd = 0; fd < 256; ++fd) {
                    if (curproc->fdesc[fd].head_cluster) continue;
                    curproc->fdesc[fd].head_cluster = clus_id;
                    tf->rax = fd;
                    return;
                }
                // no available file descriptor
                tf->rax = -2;
                return;
            }
            ind = -1;
        }
    }
    tf->rax = -3; // filename too long
}

void
sys_read(struct ProcContext *tf)
{
    printk("sys_read() not implemented yet.\n");
}

void
syscall(struct ProcContext *tf)
{
    int num = tf->rax;
    switch (num) {
    case 1:
        curproc->state = PENDING;
        kill_proc(curproc);
        sched();
    case 2:
        sys_hello();
        tf->rax = 0;
        break;
    case 3:
        console_putch(tf->rdx);
        break;
    case 4:
        sys_fork(tf);
        break;
    case 5:
        sys_open(tf);
        break;
    case 6:
        sys_read(tf);
        break;
    default:
        printk("unknown syscall\n");
        while (1);
    }
}