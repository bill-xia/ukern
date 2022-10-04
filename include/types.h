#ifndef TYPES_H
#define TYPES_H

typedef unsigned char uint8_t;
typedef char int8_t;
typedef unsigned short uint16_t;
typedef short int16_t;
typedef unsigned uint32_t;
typedef int int32_t;
typedef unsigned long uint64_t;
typedef long int64_t;
typedef uint32_t size_t;

typedef uint64_t pml4e_t;
typedef uint64_t pdpte_t;
typedef uint64_t pde_t;
typedef uint64_t pte_t;

typedef uint64_t* pgtbl_t;
typedef uint64_t* pdpt_t;
typedef uint64_t* pd_t;
typedef uint64_t* pt_t;

#define NULL 0

#define E_NOMEM 1

#define E_FORMAT 2 // for loading program image
#define E_NOPCB 3  // for creating proc

#endif