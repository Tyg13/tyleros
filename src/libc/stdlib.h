#ifndef LIBC_STDLIB_H
#define LIBC_STDLIB_H

#include "platform_specific.h"
#include <stddef.h>

LIBC_NAMESPACE_BEGIN

LIBC_NORETURN void abort(void);
LIBC_NORETURN void exit(int exit_code);
void *malloc(size_t size);
void free(void *);

void *aligned_alloc(size_t alignment, size_t size);

int atexit(void (*)(void));
int atoi(const char *);
char *itoa(int value, char *str, int base);
char *getenv(const char *);

#define EXIT_FAILURE -1
#define EXIT_SUCCESS 0

LIBC_NAMESPACE_END

#endif
