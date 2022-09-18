#include "intr.h"
#include "types.h"
#include "mem.h"
#include "x86.h"
#include "ukernio.h"

struct IDTGateDesc *idt;
struct IDTDesc idt_desc;

void _divide_error_entry(void);

void (*intr_entry[])(void) = {
    _divide_error_entry,
};

void
init_intr(void)
{
    idt = (struct IDTGateDesc *)
        ROUNDUP((uint64_t)end_kmem, sizeof(struct IDTGateDesc));
    end_kmem = (char *)(idt + NIDT);
    for (int i = 0; i < NIDT; ++i) {
        struct IDTGateDesc tmp = {
            .offset1 = (uint64_t)intr_entry[i] & MASK(16),
            .seg_sel = KERN_CODE_SEL,
            .ist = 0,
            .zeros = 0,
            .type = 15, // 14 - interrupt gate, 15 - trap gate
            .S = 0,
            .DPL = 3, // user
            .P = 1,
            .offset2 = ((uint64_t)intr_entry[i] >> 16) & MASK(32),
            .offset3 = (uint64_t)intr_entry[i] >> 32,
            .reserved = 0
        };
        idt[i] = tmp;
    }
    struct IDTDesc tmp = {
        .limit = 16 * NIDT - 1,
        .addr = (uint64_t)idt
    };
    idt_desc = tmp;
    lidt(&idt_desc);
    // sti();
}

void divide_error_handler(void) {
    printk("divide error :(\n");
    while (1);
}