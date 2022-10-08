#include "string.h"

void *
memcpy(void *s1, const void *s2, size_t n)
{
    char *src = s2, *dst = s1;
    for (int i = 0; i < n; ++i) {
        dst[i] = src[i];
    }
}

void *
memset(void *s, int c, size_t n)
{
    char *dst = s;
    for (int i = 0; i < n; ++i) {
        dst[i] = c;
    }
}

size_t
strlen(const char *s)
{
    for (int i = 0; i < STR_MAXLEN; ++i) {
        if (s[i] == '\0') return i;
    }
    return STR_MAXLEN;
}