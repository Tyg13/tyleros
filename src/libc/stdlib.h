#ifndef LIBC_STDLIB_H
#define LIBC_STDLIB_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void abort(void);
void *malloc(size_t);
void free(void *);

int atexit(void (*)(void));
int atoi(const char *);
char *itoa(int value, char *str, int base);
char *getenv(const char *);

#define EXIT_FAILURE -1
#define EXIT_SUCCESS 0

#ifdef __cplusplus
}
#endif

#endif
