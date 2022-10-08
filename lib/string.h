#ifndef STRING_H
#define STRING_H

#include "types.h"

#define STR_MAXLEN 4096

void *memcpy(void *s1, const void *s2, size_t n);
void *memset(void *s, int c, size_t n);
size_t strlen(const char *s);

#endif