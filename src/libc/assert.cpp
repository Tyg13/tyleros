#include "assert.h"
#include <stdio.h>

void _handle_assert(const char *file, unsigned line, const char *condition) {
  fprintf(stderr, "assertion failed: %s:%d: `%s`\n", file, line, condition);
  asm volatile ("int3" ::: "memory");
  __builtin_unreachable();
}
_assert_handler _on_assert = &_handle_assert;
