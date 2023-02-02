#ifndef SSE_H
#define SSE_H

#define CR0_EM		(1 << 2)
#define CR0_MP		(1 << 1)
#define CR4_OSFXSR	(1 << 9)
#define CR4_OSXMMEXCPT	(1 << 10)

#define MXCSR_MASKALL	(MASK(6) << 7)

int init_sse();

#endif