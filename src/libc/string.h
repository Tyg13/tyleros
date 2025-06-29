#ifndef LIBC_STRING_H
#define LIBC_STRING_H

#include "platform_specific.h"
#include <stddef.h>

#ifndef NULL
#if defined(__cplusplus)
#define NULL 0
#else
#define NULL ((void*)0)
#endif
#endif

LIBC_NAMESPACE_BEGIN

// Memory manipulation
void *memchr(const void *ptr, int ch, size_t count);
int memcmp(const void *lhs, const void *rhs, size_t count);
void *memset(void *dst, int c, size_t n);
void *memcpy(void *LIBC_RESTRICT dst, const void *LIBC_RESTRICT src, size_t n);
void *memmove(void *dst, const void *src, size_t n);
char *memstr(const void *src, const char *needle, size_t count);

// String examination
size_t strlen(const char *);
int strcmp(const char *str1, const char *str2);
int strncmp(const char *str1, const char *str2, size_t n);

// String manipulation
char *strcpy(char *LIBC_RESTRICT dst, const char *LIBC_RESTRICT src);
char *strncpy(char *LIBC_RESTRICT dst, const char *LIBC_RESTRICT src, size_t n);
char *strcat(char *LIBC_RESTRICT dest, const char *LIBC_RESTRICT src);
char *strncat(char *LIBC_RESTRICT dest, const char *LIBC_RESTRICT src, size_t n);
char *strchr(const char *str, int character);
char *strstr(const char *src, const char *sub);

char *strrev(char *src, int len);

LIBC_NAMESPACE_END

#endif
