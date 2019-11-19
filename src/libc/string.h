#ifndef LIBC_STRING_H
#define LIBC_STRING_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

volatile void * memcpy_v(volatile void * src, const volatile void * dst, size_t n);
volatile void * memset_v(volatile void * dst, int c, size_t n);
void * memcpy(void * src, const void * dst, size_t n);
void * memset(void * dst, int c, size_t n);

char * strchr(const char * str, int character);
int    strcmp(const char * str1, const char * str2);
char * strncpy(char * dst, const char * src, size_t n);
char * strcpy(char * dst, const char * src);
size_t strlen(const char *);
char * strrev(char * src, int len);

#ifdef __cplusplus
}
#endif

#endif
