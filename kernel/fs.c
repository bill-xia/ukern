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
    printk("get_fat_at(%x)\n", id);
    ide_read(fsinfo->fat_offset + (id >> 7), fat, 1);
    return fat[id & 127];
}