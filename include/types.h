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

#define NULL 0

#define E_NOMEM 1

#define E_FORMAT 2 // for loading program image
#define E_NOPCB 3  // for creating proc

#endif