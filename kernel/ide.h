#ifndef IDE_H
#define IDE_H

void init_ide(void);
int ide_probe_disk1(void);
void ide_set_disk(int d);
int ide_read(uint32_t secno, void *dst, size_t nsecs);
int ide_write(uint32_t secno, const void *src, size_t nsecs);

#endif