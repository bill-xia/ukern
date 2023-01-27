#ifndef INIT_H
#define INIT_H

#include "console.h"
#include "mem.h"

struct boot_args {
        struct screen *screen;
        struct mem_map *mem_map;
};

void init(struct boot_args *);

#endif