#ifndef DIRENT_H
#define DIRENT_H

#include "types.h"

enum {
	DT_BLK = 1,
	DT_CHR,
	DT_DIR,
	DT_FIFO,
	DT_LNK,
	DT_REG,
	DT_SOCK,
	DT_UNKNOWN
};

typedef i32 ino_t;

struct dirent {
	ino_t	d_ino;
	u64	file_len;
	char	d_name[256],
		d_type;
};

#endif