#ifndef TYPES_H
#define TYPES_H

typedef unsigned char	u8;
typedef char		i8;
typedef unsigned short	u16;
typedef short		i16;
typedef unsigned	u32;
typedef int		i32;
typedef unsigned long	u64;
typedef long		i64;
typedef u32		size_t;

typedef u64		pml4e_t;
typedef u64		pdpte_t;
typedef u64		pde_t;
typedef u64		pte_t;

typedef u64*		pgtbl_t;
typedef u64*		pdpt_t;
typedef u64*		pd_t;
typedef u64*		pt_t;

#define NULL 0

#endif