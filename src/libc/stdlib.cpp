#include "stdlib.h"
#include "platform_specific.h"
#include "stdio.h"

LIBC_NAMESPACE_BEGIN

char *itoa(int value, char *str, int base) {
  switch (base) {
  case 10:
    sprintf(str, "%d", value);
    return str;
  case 16:
    sprintf(str, "%x", value);
    return str;
  default:
    // unimplemented
    return NULL;
  }
}

void abort(void) {
  _syscall0(_SYSCALL_ABORT);
  LIBC_UNREACHABLE();
}

void exit(int exit_code) {
  _syscall1(_SYSCALL_EXIT, (uint64_t)exit_code);
  LIBC_UNREACHABLE();
}

void *malloc(size_t size) { return aligned_alloc(size, 1); }
void free(void *ptr) { _syscall1(_SYSCALL_FREE, (uint64_t)ptr); }

void *aligned_alloc(size_t size, size_t alignment) {
  return (void*)_syscall2(_SYSCALL_ALLOC, (uint64_t)size, (uint64_t)alignment);
}

LIBC_NAMESPACE_END
