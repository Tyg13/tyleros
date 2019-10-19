#ifndef LIBC_STDIO_H
#define LIBC_STDIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stddef.h>

#define SEEK_SET 0

typedef struct FILE { int dummy; } FILE;
extern FILE * stderr;
#define stderr stderr

int fclose(FILE *);
int fflush(FILE *);
FILE * fopen(const char *, const char *);
int fprintf(FILE *, const char *, ...);
size_t fread(void *, size_t, size_t, FILE *);
int fseek(FILE *, long, int);
long ftell(FILE *);
size_t fwrite(const void *, size_t, size_t, FILE *);
void setbuf(FILE *, char *);
int vfprintf(FILE *, const char *, va_list);

int sprintf(char * str, const char * fmt, ...);

#ifdef __cplusplus
}
#endif

#endif
