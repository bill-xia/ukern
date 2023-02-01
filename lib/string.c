#include "string.h"

void *
memcpy(void *s1, const void *s2, size_t n)
{
	const char *src = s2;
	char *dst = s1;
	for (int i = 0; i < n; ++i) {
		dst[i] = src[i];
	}
	return dst;
}

void *
memset(void *s, int c, size_t n)
{
	char *dst = s;
	for (int i = 0; i < n; ++i) {
		dst[i] = c;
	}
	return dst;
}

size_t
strlen(const char *s)
{
	for (int i = 0; i < STR_MAXLEN; ++i) {
		if (s[i] == '\0') return i;
	}
	return STR_MAXLEN;
}

int
strcmp(const char *s1, const char *s2, size_t n)
{
	for (int i = 0; i < n; ++i) {
		if (s1[i] != s2[i]) {
			return 1;
		}
		if (s1[i] == '\0') // && s2[i] == '\0'
			break;
	}
	return 0;
}

void
strncpy(char *dst, char *src, int n)
{
	for (int i = 0; i < n && *src != '\0'; ++i) {
		*dst = *src;
		src++;
		dst++;
	}
	*dst = '\0';
}

void
strcpy(char *dst, char *src)
{
	for (int i = 0; *src != '\0'; ++i) {
		*dst = *src;
		src++;
		dst++;
	}
	*dst = '\0';
}

char *
strcat(char *dest, const char *src)
{
	size_t dest_len = strlen(dest);
	size_t i;

	for (i = 0 ; src[i] != '\0' ; i++)
		dest[dest_len + i] = src[i];
	dest[dest_len + i] = '\0';

	return dest;
}