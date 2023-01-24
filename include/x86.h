#ifndef JOS_INC_X86_H
#define JOS_INC_X86_H

#include "types.h"

#define RFLAGS_IF 0x200
#define RFLAGS_IOPL_SHIFT 12

static inline void
breakpoint(void)
{
	asm volatile("int3");
}

static inline u8
inb(int port)
{
	u8 data;
	asm volatile("inb %w1,%0" : "=a" (data) : "d" (port));
	return data;
}

static inline void
insb(int port, void *addr, int cnt)
{
	asm volatile("cld\n\trepne\n\tinsb"
		     : "=D" (addr), "=c" (cnt)
		     : "d" (port), "0" (addr), "1" (cnt)
		     : "memory", "cc");
}

static inline u16
inw(int port)
{
	u16 data;
	asm volatile("inw %w1,%0" : "=a" (data) : "d" (port));
	return data;
}

static inline void
insw(int port, void *addr, int cnt)
{
	asm volatile("cld\n\trepne\n\tinsw"
		     : "=D" (addr), "=c" (cnt)
		     : "d" (port), "0" (addr), "1" (cnt)
		     : "memory", "cc");
}

static inline u32
inl(int port)
{
	u32 data;
	asm volatile("inl %w1,%0" : "=a" (data) : "d" (port));
	return data;
}

static inline void
insl(int port, void *addr, int cnt)
{
	asm volatile("cld\n\trepne\n\tinsl"
		     : "=D" (addr), "=c" (cnt)
		     : "d" (port), "0" (addr), "1" (cnt)
		     : "memory", "cc");
}

static inline void
outb(int port, u8 data)
{
	asm volatile("outb %0,%w1" : : "a" (data), "d" (port));
}

static inline void
outsb(int port, const void *addr, int cnt)
{
	asm volatile("cld\n\trepne\n\toutsb"
		     : "=S" (addr), "=c" (cnt)
		     : "d" (port), "0" (addr), "1" (cnt)
		     : "cc");
}

static inline void
outw(int port, u16 data)
{
	asm volatile("outw %0,%w1" : : "a" (data), "d" (port));
}

static inline void
outsw(int port, const void *addr, int cnt)
{
	asm volatile("cld\n\trepne\n\toutsw"
		     : "=S" (addr), "=c" (cnt)
		     : "d" (port), "0" (addr), "1" (cnt)
		     : "cc");
}

static inline void
outsl(int port, const void *addr, int cnt)
{
	asm volatile("cld\n\trepne\n\toutsl"
		     : "=S" (addr), "=c" (cnt)
		     : "d" (port), "0" (addr), "1" (cnt)
		     : "cc");
}

static inline void
outl(int port, u32 data)
{
	asm volatile("outl %0,%w1" : : "a" (data), "d" (port));
}

static inline void
invlpg(void *addr)
{
	asm volatile("invlpg (%0)" : : "r" (addr) : "memory");
}

static inline void
lidt(void *p)
{
	asm volatile("lidt (%0)" : : "r" (p));
}

static inline void
lgdt(void *p)
{
	asm volatile("lgdt (%0)" : : "r" (p));
}

static inline void
lldt(u16 sel)
{
	asm volatile("lldt %0" : : "r" (sel));
}

static inline void
ltr(u16 sel)
{
	asm volatile("ltr %0" : : "r" (sel));
}

static inline void
lcr0(u64 val)
{
	asm volatile("movq %0,%%cr0" : : "r" (val));
}

static inline u64
rcr0(void)
{
	u64 val;
	asm volatile("movq %%cr0,%0" : "=r" (val));
	return val;
}

static inline u64
rcr2(void)
{
	u64 val;
	asm volatile("movq %%cr2,%0" : "=r" (val));
	return val;
}

static inline void
lcr3(u64 val)
{
	asm volatile("movq %0,%%cr3" : : "r" (val));
}

static inline u64
rcr3(void)
{
	u64 val;
	asm volatile("movq %%cr3,%0" : "=r" (val));
	return val;
}

static inline void
lcr4(u64 val)
{
	asm volatile("movq %0,%%cr4" : : "r" (val));
}

static inline u64
rcr4(void)
{
	u64 cr4;
	asm volatile("movq %%cr4,%0" : "=r" (cr4));
	return cr4;
}

static inline void
tlbflush(void)
{
	u64 cr3;
	asm volatile("movq %%cr3,%0" : "=r" (cr3));
	asm volatile("movq %0,%%cr3" : : "r" (cr3));
}

static inline u64
read_eflags(void)
{
	u64 eflags;
	asm volatile("pushfq; popq %0" : "=r" (eflags));
	return eflags;
}

static inline void
write_eflags(u64 eflags)
{
	asm volatile("pushq %0; popfq" : : "r" (eflags));
}

static inline u64
read_ebp(void)
{
	u64 ebp;
	asm volatile("movq %%ebp,%0" : "=r" (ebp));
	return ebp;
}

static inline u64
read_esp(void)
{
	u64 esp;
	asm volatile("movq %%esp,%0" : "=r" (esp));
	return esp;
}

static inline void
cpuid(u64 info, u64 *raxp, u64 *rbxp, u64 *rcxp, u64 *rdxp)
{
	u64 rax, rbx, rcx, rdx;
	asm volatile("cpuid"
		     : "=a" (rax), "=b" (rbx), "=c" (rcx), "=d" (rdx)
		     : "a" (info));
	if (raxp)
		*raxp = rax;
	if (rbxp)
		*rbxp = rbx;
	if (rcxp)
		*rcxp = rcx;
	if (rdxp)
		*rdxp = rdx;
}

static inline void
sti(void)
{
	asm volatile("sti" :::);
}


static inline u64
read_tsc(void)
{
	u64 tsc;
	asm volatile("rdtsc" : "=A" (tsc));
	return tsc;
}

static inline u64
xchg(volatile u64 *addr, u64 newval)
{
	u64 result;

	// The + in "+m" denotes a read-modify-write operand.
	asm volatile("lock; xchgl %0, %1"
		     : "+m" (*addr), "=a" (result)
		     : "1" (newval)
		     : "cc");
	return result;
}

static inline u64
rdmsr(u64 addr)
{
	u64 result;
	asm volatile("mov %0, %%rcx; rdmsr;"
			 : "=a" (result)
			 : "r" (addr));
	return result;
}

#endif /* !JOS_INC_X86_H */
