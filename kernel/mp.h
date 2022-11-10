#ifndef MP_H
#define MP_H

#define APIC_BASE 0xFFFFFFFFFFFFE000
#define APIC_ESR  0x0280
#define APIC_TIMER 0x0320
#define APIC_TIMER_INIT_COUNT 0x0380
#define APIC_TIMER_CURR_COUNT 0x0390

void init_mp(void);
void lapic_eoi(void);

#endif