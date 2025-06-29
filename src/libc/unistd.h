#ifndef LIBC_UNISTD_H
#define LIBC_UNISTD_H

#include "platform_specific.h"
#include <sys/types.h>

LIBC_NAMESPACE_BEGIN

int execv(const char *, char * const[]);
int execve(const char *, char * const[], char * const[]);
int execvp(const char *, char * const[]);
pid_t fork(void);

LIBC_NAMESPACE_END

#endif
