#include "assert.h"
#include "stdlib.h"

#if !LIBC_IN_KERNEL
#include "stdio.h"
#include "platform_specific.h"
#endif

#if TESTING_LIBC
using namespace kstd;
#endif

#if LIBC_IN_KERNEL
namespace kstd {
[[noreturn]] __attribute__((cold)) void panic(const char *msg, ...);
} // namespace kstd
[[noreturn]]
__attribute__((cold)) static void handle_assert(const char *file, unsigned line,
                                                const char *condition) {
  kstd::panic("assertion failed: %s:%d: `%s`\n", file, line, condition);
}
#else
[[noreturn]]
__attribute__((cold)) static void handle_assert(const char *file, unsigned line,
                                                const char *condition) {
  printf("assertion failed: %s:%d: `%s`\n", file, line, condition);
  abort();
}
#endif

_assert_handler _on_assert = &handle_assert;
