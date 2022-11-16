#ifndef MP_H
#define MP_H

// Local APIC registers, divided by 4 for use as uint32_t[] indices.
#define LAPIC_BASE      0xFFFFFFFFFFFFD000 // Base kernel address for lapic registers
#define LAPICR_ID      (0x0020/4)   // ID
#define LAPICR_VER     (0x0030/4)   // Version
#define LAPICR_TPR     (0x0080/4)   // Task Priority
#define LAPICR_EOI     (0x00B0/4)   // EOI
#define LAPICR_SVR     (0x00F0/4)   // Spurious Interrupt Vector
	#define LAPIC_ENABLE     0x00000100   // Unit Enable
#define LAPICR_ESR     (0x0280/4)   // Error Status
#define LAPICR_ICRLO   (0x0300/4)   // Interrupt Command
	#define LAPIC_INIT       0x00000500   // INIT/RESET
	#define LAPIC_STARTUP    0x00000600   // Startup IPI
	#define LAPIC_DELIVS     0x00001000   // Delivery status
	#define LAPIC_ASSERT     0x00004000   // Assert interrupt (vs deassert)
	#define LAPIC_DEASSERT   0x00000000
	#define LAPIC_LEVEL      0x00008000   // Level triggered
	#define LAPIC_BCAST      0x00080000   // Send to all APICs, including self.
	#define LAPIC_OTHERS     0x000C0000   // Send to all APICs, excluding self.
	#define LAPIC_BUSY       0x00001000
	#define LAPIC_FIXED      0x00000000
#define LAPICR_ICRHI   (0x0310/4)   // Interrupt Command [63:32]
#define LAPICR_TIMER   (0x0320/4)   // Local Vector Table 0 (TIMER)
	#define LAPIC_X1         0x0000000B   // divide counts by 1
	#define LAPIC_PERIODIC   0x00020000   // Periodic
#define LAPICR_PCINT   (0x0340/4)   // Performance Counter LVT
#define LAPICR_LINT0   (0x0350/4)   // Local Vector Table 1 (LINT0)
#define LAPICR_LINT1   (0x0360/4)   // Local Vector Table 2 (LINT1)
#define LAPICR_ERROR   (0x0370/4)   // Local Vector Table 3 (ERROR)
	#define LAPIC_MASKED     0x00010000   // Interrupt masked
#define LAPICR_TICR    (0x0380/4)   // Timer Initial Count
#define LAPICR_TCCR    (0x0390/4)   // Timer Current Count
#define LAPICR_TDCR    (0x03E0/4)   // Timer Divide Configuration

void init_mp(void);
void lapic_eoi(void);

#endif