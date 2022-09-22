#ifndef INTR_H
#define INTR_H

#include "types.h"

#define NIDT 32

struct IDTGateDesc {
    uint16_t offset1;
    uint16_t seg_sel;
    uint8_t ist : 3;
    uint8_t zeros : 5;
    uint8_t type : 4; // present and DPL
    uint8_t S : 1; // should be zero
    uint8_t DPL: 2;
    uint8_t P : 1; // present
    uint16_t offset2;
    uint32_t offset3;
    uint32_t reserved;
};

struct IDTDesc{
    uint16_t limit;
    uint64_t addr;
} __attribute__ ((packed));

void init_intr(void);

#endif