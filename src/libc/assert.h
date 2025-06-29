#ifndef LIBC_ASSERT_H
#define LIBC_ASSERT_H

#include "platform_specific.h"

#ifndef assert
#ifdef NDEBUG
#define assert(x) (void)(x)
#else
#define assert(x)                                                              \
  do {                                                                         \
    if (!(x)) {                                                                \
      (*::LIBC_NAMESPACE_PREFIX _on_assert)(__FILE__, __LINE__, #x);           \
    }                                                                          \
  } while (0)
#endif
#endif

LIBC_NAMESPACE_BEGIN

typedef void (*_assert_handler)(const char *file, unsigned line,
                                const char *condition);
extern _assert_handler _on_assert;

LIBC_NAMESPACE_END

#endif
