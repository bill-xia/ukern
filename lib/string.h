#ifndef STRING_H
#define STRING_H

#include "types.h"

#define STR_MAXLEN 4096

void *memcpy(void *dst, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
size_t strlen(const char *s);
int strcmp(const char *s1, const char *s2, size_t n);

#endif