#ifndef LIBC_STDIO_H
#define LIBC_STDIO_H

#include "./platform_specific.h"
#include <stdarg.h>
#include <stddef.h>

LIBC_NAMESPACE_BEGIN

#ifndef SEEK_SET
#define SEEK_SET 0
#endif
#ifndef SEEK_END
#define SEEK_END -1
#endif
#ifndef EOF
#define EOF (int)-1
#endif
#ifndef stdout
#define stdout stdout
#endif
#ifndef stderr
#define stderr stderr
#endif

struct FILE;
typedef struct FILE FILE;
extern FILE *stdout;
extern FILE *stderr;

int fclose(FILE *);
int fflush(FILE *);
FILE *fopen(const char *, const char *);
size_t fread(void *, size_t, size_t, FILE *);
int fseek(FILE *, long, int);
long ftell(FILE *);
size_t fwrite(const void *, size_t, size_t, FILE *);
int feof(FILE *);
int ferror(FILE *);

int printf(const char *, ...)
    __attribute__((format(printf, 1, 2)));
int vprintf(const char *, va_list args);
int fprintf(FILE *, const char *, ...)
    __attribute__((format(printf, 2, 3)));
int vfprintf(FILE *, const char *, va_list);

int fputs(const char *str, FILE *stream);
int puts(const char *str);

void setbuf(FILE *, char *);

int vsprintf(char *str, const char *fmt, va_list args);
int sprintf(char *str, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));
int vsnprintf(char *str, size_t buf_size, const char *fmt, va_list args);
int snprintf(char *str, size_t buf_size, const char *fmt, ...)
    __attribute__((format(printf, 3, 4)));

void perror(const char *);

LIBC_NAMESPACE_END

#endif
